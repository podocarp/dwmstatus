/*
 * Copy me if you can.
 * by 20h
 */

#define _BSD_SOURCE
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include <X11/Xlib.h>

char* zone = "Asia/Singapore";
static Display* dpy;

char* smprintf(char* fmt, ...)
{
    va_list fmtargs;
    char* ret;
    int len;

    va_start(fmtargs, fmt);
    len = vsnprintf(NULL, 0, fmt, fmtargs);
    va_end(fmtargs);

    ret = malloc(++len);
    if (ret == NULL) {
        perror("malloc");
        exit(1);
    }

    va_start(fmtargs, fmt);
    vsnprintf(ret, len, fmt, fmtargs);
    va_end(fmtargs);

    return ret;
}

void settz(char* tzname)
{
    setenv("TZ", tzname, 1);
}

char* mktimes(char* fmt, char* tzname)
{
    char buf[129];
    time_t tim;
    struct tm* timtm;

    settz(tzname);
    tim = time(NULL);
    timtm = localtime(&tim);
    if (timtm == NULL)
        return smprintf("");

    if (!strftime(buf, sizeof(buf) - 1, fmt, timtm)) {
        fprintf(stderr, "strftime == 0\n");
        return smprintf("");
    }

    return smprintf("%s", buf);
}

void setstatus(char* str)
{
    XStoreName(dpy, DefaultRootWindow(dpy), str);
    XSync(dpy, False);
}

char* loadavg(void)
{
    double avgs[3];

    if (getloadavg(avgs, 3) < 0)
        return smprintf("");

    return smprintf("%.2f %.2f %.2f", avgs[0], avgs[1], avgs[2]);
}

char* readfile(char* base, char* file)
{
    char *path, line[513];
    FILE* fd;

    memset(line, 0, sizeof(line));

    path = smprintf("%s/%s", base, file);
    fd = fopen(path, "r");
    free(path);
    if (fd == NULL)
        return NULL;

    if (fgets(line, sizeof(line) - 1, fd) == NULL)
        return NULL;
    fclose(fd);

    return smprintf("%s", line);
}

char* getbattery(char* base)
{
    char *co, status;
    float descap = -1, remcap = -1, power = 0;
    
    descap = -1;
    remcap = -1;

    co = readfile(base, "energy_full");
    if (co == NULL)
        return smprintf("");
    sscanf(co, "%f", &descap);
    free(co);

    co = readfile(base, "energy_now");
    if (co == NULL)
        return smprintf("");
    sscanf(co, "%f", &remcap);
    free(co);

    co = readfile(base, "power_now");
    if (co == NULL)
        return smprintf("");
    sscanf(co, "%f", &power);
    free(co);

    co = readfile(base, "status");
    if (!strncmp(co, "Discharging", 11)) {
        status = '-';
    } else if (!strncmp(co, "Charging", 8)) {
        status = '+';
    } else {
        status = '=';
    }

    if (remcap < 0 || descap < 0)
        return smprintf("?");

    if (power == 0)
        return smprintf("%.0f%%%c", (remcap / descap) * 100, status);
    else {
        return smprintf("%.0f%%%c (%.2f)", (remcap / descap) * 100, status, remcap / power);
    }
}

char* gettemperature(char* base, char* sensor)
{
    char* co;

    co = readfile(base, sensor);
    if (co == NULL)
        return smprintf("");
    return smprintf("%02.0f°C", atof(co) / 1000);
}

int main(void)
{
    //snd_mixer_elem_t* alsaelem = initalsa();

    char* status;
    char* time;
    char* bat;
    char* bat1;
    char* t0;
    //char* volume;

    if (!(dpy = XOpenDisplay(NULL))) {
        fprintf(stderr, "dwmstatus: cannot open display.\n");
        return 1;
    }
    for (;; sleep(30)) {
        //volume = getvol(alsaelem);
        t0 = gettemperature("/sys/class/thermal/thermal_zone19", "temp");
        bat = getbattery("/sys/class/power_supply/BAT0");
        bat1 = getbattery("/sys/class/power_supply/BAT1");
        time = mktimes("%a %d %b %Y  %H:%M", zone);

        if (bat1[0] == '\0')
                status = smprintf("%s  %s  %s", t0, bat, time);
        else
                status = smprintf("%s  %s %s  %s", t0, bat, bat1, time);
        setstatus(status);
        free(bat);
        free(bat1);
        free(time);
        free(t0);
        free(status);
    }

    XCloseDisplay(dpy);
    return 0;
}
