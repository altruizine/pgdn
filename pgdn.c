#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <linux/input.h>
#include <linux/uinput.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/time.h>

#define FIFONAME "/app-cache/.uinput-virtual-keyboard-fifo"

#define KEYMASK 0xfff;
#define SHIFT 0x10000

static int MAP102=0;
static int MAP116=0;

#define die(str, args...) do { \
        perror(str); \
        unlink(FIFONAME); \
        exit(EXIT_FAILURE); \
    } while(0)

int k114 = 114;
int k115 = 115;

struct profile {
    char name[256];
    int pid;
    int t114;
    int t115;
    int t102;
    int t116;
    struct profile *next;
} *profiles, *defaultprofile;

struct timeval lastreadpids;

struct profile * addprofile(char *name, int t114, int t115, int t102, int t116) {
    struct profile *newprof;

    newprof=profiles;
    while(newprof && strcmp(newprof->name,name)) newprof = newprof->next;

    if(!newprof) {
        newprof = (struct profile *)malloc(sizeof(struct profile));
        newprof->pid = -1;
        strncpy(newprof->name, name, 255);
        newprof->next = profiles;
        profiles=newprof;
    }

    newprof->t114 = t114;
    newprof->t115 = t115;
    newprof->t102 = t102;
    newprof->t116 = t116;
    return newprof;
}

void sendkey(int fd, int key, int value) {
    struct input_event oev;
    if (value && (SHIFT & key)) {
        sendkey(fd, KEY_LEFTSHIFT, 1);
    }
    memset(&oev, 0, sizeof(struct input_event));
    oev.type = EV_KEY;
    oev.code = key&KEYMASK;
    oev.value = value;
    if(write(fd, &oev, sizeof(struct input_event)) < 0)
        die("error: write");

    if (!value && (SHIFT & key)) {
        sendkey(fd, KEY_LEFTSHIFT, 0);
    }
}

void setpids() {
    struct profile *p;
    DIR *dir;
    struct dirent * de;

    dir=opendir("/proc");
    if(!dir) {
        return ;
    }

    while((de=readdir(dir))) {
        if(isdigit(de->d_name[0])) {
            char n[256];
            FILE *f;
            snprintf(n,255,"/proc/%s/cmdline",de->d_name);

            f=fopen(n,"r");
            if(f) {
                fread(n,255,1,f);
                fclose(f);
                p=profiles;
                while( p && strcmp(p->name,n)) p = p->next;
                if(p) {
                    p->pid = atoi(de->d_name);
                }
            }
        }
    }
    closedir(dir);
}

int
main(void)
{
    int  fd;
    int ffd;
    int e1fd;
    int nfds;
    FILE *conf;
    struct uinput_user_dev uidev;
    fd_set fds;
    struct timeval tv1;

    daemon(0, 0);

    profiles = NULL;

    addprofile("org.mozilla.firefox", KEY_SPACE, SHIFT|KEY_SPACE, 102, 116);
    addprofile("com.android.browser", KEY_SPACE, SHIFT|KEY_SPACE, 102, 116);
    addprofile("com.quoord.tapatalkpro.activity", KEY_DOWN, KEY_UP, 102, 116);
    addprofile("com.sec.android.app.camera", 212, 212, 102, 212);
    defaultprofile = addprofile("default", KEY_VOLUMEDOWN, KEY_VOLUMEUP, 102, 116);

    gettimeofday(&tv1,NULL);
    setpids();
    gettimeofday(&lastreadpids,NULL);
//    fprintf(stderr, "took %i microsec\n",
//            (lastreadpids.tv_sec-tv1.tv_sec)*1000000+lastreadpids.tv_usec-tv1.tv_usec);
//    fprintf(stderr, "  %i %i %i\n",   profiles->pid, profiles->next->pid, profiles->next->next->pid);

    fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
    if(fd < 0)
        die("error: open");

    if(ioctl(fd, UI_SET_EVBIT, EV_KEY) < 0)
        die("error: ioctl");
#if 0
    for(i=0;i<500;i++) {
        if(ioctl(fd, UI_SET_KEYBIT, i) < 0)
            die("error: ioctl");
    }
#else
        ioctl(fd, UI_SET_KEYBIT, KEY_SPACE);
        ioctl(fd, UI_SET_KEYBIT, KEY_LEFTSHIFT);
        ioctl(fd, UI_SET_KEYBIT, KEY_VOLUMEDOWN);
        ioctl(fd, UI_SET_KEYBIT, KEY_VOLUMEUP);
        ioctl(fd, UI_SET_KEYBIT, KEY_DOWN);
        ioctl(fd, UI_SET_KEYBIT, KEY_PAGEDOWN);
        ioctl(fd, UI_SET_KEYBIT, KEY_UP);
        ioctl(fd, UI_SET_KEYBIT, KEY_PAGEUP);
        ioctl(fd, UI_SET_KEYBIT, KEY_SEND);
        ioctl(fd, UI_SET_KEYBIT, KEY_CAMERA);
#endif
    conf = fopen("/sdcard/.pgdn","r");
    if(conf) {
        char buf[1024];
        while(fgets(buf,1024,conf)) {
            int i,j,k;
            char pname[256];
            int p114, p115, p102, p116;

            if(sscanf(buf, "map %i", &j)==1) {
                if(j==102)MAP102=1;
                if(j==116)MAP116=1;
            }
            if(sscanf(buf, "key %i", &j)==1) {
                ioctl(fd, UI_SET_KEYBIT, j);
            }
            if(sscanf(buf, "keys %i %i", &j, &k)==2) {
                for(i=j;i<=k;i++) ioctl(fd, UI_SET_KEYBIT, i);
            }
            if(sscanf(buf, "profile %255s %i %i %i %i", pname, &p114, &p115, &p102, &p116)==3+MAP102+MAP116) {
                addprofile(pname, p114, p115, p102, p116);
            }
        }
        fclose(conf);
    }


    mkfifo(FIFONAME, 0777);
    ffd=open(FIFONAME, O_RDONLY|O_NDELAY);

    memset(&uidev, 0, sizeof(uidev));
    snprintf(uidev.name, UINPUT_MAX_NAME_SIZE, "uinput-virtual-keyboard");
    uidev.id.bustype = BUS_USB;
    uidev.id.vendor  = 0x04E8;
    uidev.id.product = 0x7021;
    uidev.id.version = 1;

    if(write(fd, &uidev, sizeof(uidev)) < 0)
        die("error: write");

    if(ioctl(fd, UI_DEV_CREATE) < 0)
        die("error: ioctl");

    e1fd = open("/dev/input/event1", O_RDONLY);
    if(e1fd<0)
        die("error: open /dev/input/event1");

    nfds = (ffd>e1fd ? ffd : e1fd) +1;

    while(1) {
        struct input_event ev[64];
        int r,i;
        FD_ZERO(&fds);
        FD_SET(ffd, &fds);
        FD_SET(e1fd, &fds);
        r=select(nfds, &fds, NULL, NULL, NULL);
        if((r>0)&& FD_ISSET(e1fd,&fds)) {
            int rd;
            struct profile *curprofile;
            rd = read(e1fd, ev, sizeof(struct input_event) * 64);
            for(i=0;i*sizeof(struct input_event)<rd;i++) {
                if((ev[i].type == EV_KEY) && (ev[i].value == 1) && 
                        ((ev[i].code == k114) || (ev[i].code == k115) || (MAP102 && (ev[i].code==102)) || (MAP116 && (ev[i].code==116)))
                        ) {
                    struct timeval tv;
                    gettimeofday(&tv,NULL);
                    if(tv.tv_sec-5> lastreadpids.tv_sec) {
                        setpids();
                        gettimeofday(&lastreadpids,NULL);
                    }

                    curprofile=profiles;
                    
                    while(curprofile) {
                        if(curprofile->pid>0) {
                            char fname[256];
                            FILE *f;
                            snprintf(fname,255,"/proc/%i/oom_adj");
                            f=fopen(fname,"r");
                            if(f) {
                                int oom_adj;
                                fread(fname, 255, 1, f);
                                fclose(f);
                                if(isdigit(fname[0])||fname[0]=='-') {
                                    oom_adj = atoi(fname);
                                    if(oom_adj<=0) {
                                        break;
                                    }
                                }
                            }

                        }
                        curprofile=curprofile->next;
                    }
                    if(!curprofile)curprofile=defaultprofile;
//                    fprintf(stderr, "profile: %s\n", curprofile->name);
                }
                if(ev[i].type==EV_KEY) {
                    if(ev[i].code == k114) {
                        sendkey(fd, curprofile->t114, ev[i].value);
                    }
                    if(ev[i].code == k115) {
                        sendkey(fd, curprofile->t115, ev[i].value);
                    }
                    if(MAP102 && (ev[i].code == 102)) {
                        sendkey(fd, curprofile->t102, ev[i].value);
                    }
                    if(MAP116 && (ev[i].code == 116)) {
                        sendkey(fd, curprofile->t116, ev[i].value);
                    }
                }
            }
        } else
        if((r>0)&& FD_ISSET(ffd,&fds)) {
            char buf[1024];
            char pname[256];
            int p114, p115, p102, p116;
            int rd;
            char *n;
            rd = read(ffd, buf, 1024);
            
            if(rd == 0) {
                close(ffd);
                ffd=open(FIFONAME, O_RDONLY|O_NDELAY);
                nfds = (ffd>e1fd ? ffd : e1fd) +1;
            } else 
            if((rd>4) && (strncmp(buf, "map ", 4)==0)) {
                int i;
                i=strtol(buf+4,&n, 0);
                if(i==114) {
                    int m;
                    m=0;
                    if(rd>n-buf) {
                        m=strtol(n, NULL, 0);
                    }
                    defaultprofile->t114 = m;
                }
                if(i==115) {
                    int m;
                    m=0;
                    if(rd>n-buf) {
                        m=strtol(n, NULL, 0);
                    }
                    defaultprofile->t115 = m;
                }
            }
            if((rd>5) && (strncmp(buf, "send ", 5)==0)) {
                int i;
                i=strtol(buf+5,&n, 0);
                if(i>0) {
                    sendkey(fd,i,1);
                    sendkey(fd,i,0);
                }
            }
            if((rd>10) && (sscanf(buf, "profile %255s %i %i %i %i", pname, &p114, &p115, &p102, &p116)==3+MAP102+MAP116)) {
                addprofile(pname, p114, p115, p102, p116);
            }
        }
    }

    if(ioctl(fd, UI_DEV_DESTROY) < 0)
        die("error: ioctl");

    close(e1fd);
    close(fd);
    close(ffd);
    unlink(FIFONAME);

    return 0;
}
