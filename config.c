#include "config.h"
/*		This program is free software; you can redistribute it
 *		and/or  modify it under  the terms of  the GNU General
 *		Public  License as  published  by  the  Free  Software
 *		Foundation;  either  version 2 of the License, or  (at
 *		your option) any later version.
 *
 * 		This program is distributed in the hope that it will be useful,
 * 		but WITHOUT ANY WARRANTY; without even the implied warranty of
 *		MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *		GNU General Public License for more details.
 */
extern int Max_devices;
extern struct xot_device *Devices;

extern char *Name_log_file;
extern char *Name_cfg_file;
extern char *Bindto;

extern int Log_level;
extern int Lport;
extern int Rport;
extern int FNagle;

static const char *litlvl = "log_level";
static const char *litlog = "log_path";
static const char *litrpo = "remote_port";
static const char *litlpo = "local_port";
static const char *litbnd = "bind_local_ip_address";
static const char *litnag = "algorithm_Nagle";

int tot_sect = 2;
char *sect_names[] = {"OPTIONS","DEVICES"};

void
print_cfg(void) {
	Printf(1,"log_level %i\n",Log_level);
	Printf(1,"log_path %s\n",Name_log_file);
	Printf(1,"remote_port %i     local_port %i\n",Rport,Lport);
	if (Bindto != NULL) {
		Printf(1,"Bind to %s\n",Bindto);
	}
	Printf(1,"TCP - algorithm Nagle %s\n",FNagle?"ON":"Off");

}

void print_usage() {
	Printf(0,"Usage: xotd [config-file]\n");
}

void gopt(int argc,char *argv[]) {
	if (argc == 2)  {
		//printf("argv %s\n",argv[1]);
		if (strcmp(argv[1],"-h") == 0) {
            print_usage();
            exit(1);
		}
		else if (strlen(argv[1]) < MAXFILENAME)
			strcpy(Name_cfg_file,argv[1]);
		else {
			printf("Very long name of cfg file\n");
		}
	}
	else if (argc != 1) {
		print_usage();
		exit(1);
	}
}


int
is_key(char *line) {
	//int len = strlen(line) - 2;
	char *secend = strrchr(line,']');

	if (*line == '[' && secend != NULL) {
		//printf("Seems it a key\n");
		line++;
		//*(line + len - 1) = 0;
		*secend = 0;
		int startkey = 1;
		if (*line == '-') {
			//printf("Seems it end key\n");
			startkey = -1;
			line++;
		}
		int i;
		for (i = 0;i < tot_sect;i++) {
			if (strcmp(line,sect_names[i]) == 0) {
				//printf("it`s a %i  sect\n",i+1);
				break;
			}
		}
		if (i == tot_sect) {
			return 0;
		}
		return (++i)*startkey;
	}
	else {
		return 0;
	}
}

void
read_cfg (void) {
	FILE *f;
	char line [512];

	if (!(f = fopen (Name_cfg_file, "r"))) {
		Printf(0,"Cant open config file %s\n",Name_cfg_file);
		exit(1);
	}

	int nowsect = 0;
	unsigned int vectot = 0;
	while (fgets (line, sizeof line, f)) {
		if (*line == '#' || strlen(line) < 2) continue;
	
		//printf("ini line: %s",line);
		int kod = is_key(line);
		//printf("kod %i\n",kod);
		if (nowsect == 0 && kod > 0) {
			//printf("start sect\n");
			nowsect = kod;
		}
		else if (nowsect != 0 && kod < 0 && nowsect == abs(kod)) {
			//printf("end sect\n");
			nowsect = 0;
		}
		else if (kod == 0 && nowsect != 0) {
			//printf("for sect %i %s",nowsect,line);
			switch (nowsect ) {
				case 2: {
					char *device_name = strtok (line, " \t\n");
					char *remote_name = strtok (NULL, " \t\n");
					char *circuits = strtok (NULL, " \t\n");
			
					if (!device_name || *device_name == '#') continue;
			
					if (!remote_name) {
						Printf(0,"Bad line %s in %s\n", device_name, Name_cfg_file);
						continue;
					}
					config_device (device_name, remote_name, circuits);
			
				}
				break;
				case 1: {
					char *lit = strtok (line, " \t\n");
					char *val = strtok (NULL, " \t\n");
					if (lit != NULL && val != NULL) {
						//printf("%s      %s\n",lit,val);
						if (strcmp(lit,litlvl) == 0) {
							Log_level = atoi(val);
							if (Log_level < 0) Log_level = 0;
							if (Log_level > 10) Log_level = 10;
						}
						else if (strcmp(lit,litlog) == 0) {
							strncpy(Name_log_file,val,MAXFILENAME);
							*(Name_log_file + MAXFILENAME - 1) = 0;
						}
						else if (strcmp(lit,litlpo) == 0) {
							Lport = atoi(val);
						}
						else if (strcmp(lit,litrpo) == 0) {
							Rport = atoi(val);
						}
						else if (strcmp(lit,litbnd) == 0) {
							Bindto = malloc(200);
							if (Bindto == NULL) {
								Perror("Unable alloc memory:");
								exit(1);
							}
							strncpy(Bindto,val,MAXFILENAME);
							*(Bindto + MAXFILENAME - 1) = 0;
						}
						else if (strcmp(lit,litnag) == 0) {
							if (strcasecmp(val,"off") == 0) {
								FNagle = 0;
							}
						}
						else {
						}
					}
				}
				break;
			}
		}
		else {
		//printf("Ignore line \n");
		}
	}
}

static int unit_of_devname(const char *devname) {
	int len;

	if (!devname || !*devname)
		return -1;
	len = strlen(devname);
	while (len>0 && isdigit(devname[len-1]))
		len--;
	return atoi(devname+len);
}



void config_device (char *device_name, char *remote_name, char *circuits) {
	//printf("config_device: %s %s %s\n",device_name,remote_name,circuits);
	struct hostent *host;
	int n;

	int unit = unit_of_devname (device_name);//num from x25tap0 - 0
	int vc;

	struct xot_device *dev;

	if (unit < 0 || unit > MAX_LINKS - NETLINK_TAPBASE - 1) {
		Printf(0, "Invalid netlink tap unit %s\n",device_name);
		return;
	}

	if (!(host = gethostbyname (remote_name))) {
		Printf(0, "Can't find %s for %s\n",remote_name, device_name);
		return;
	}

	if (!circuits) {
		vc = 256;
	}
	else if (sscanf (circuits, "%d", &vc) != 1 || vc <= 0 || vc > 4095) {
		Printf(0, "Bad vc's %s for %s\n",circuits, device_name);
		return;
	}

	++Max_devices;

	Devices = realloc (Devices, Max_devices * sizeof *Devices);

	dev = &Devices[Max_devices - 1];
	dev->tap = unit;
	dev->max_xot = vc;
	dev->xot = calloc (dev->max_xot, sizeof *dev->xot);

	for (n = 0; host->h_addr_list[n]; ++n);//how much addrs match name  remote_name
	//printf("host->h_addr_list[n] n: %i\n",n);

	dev->addr = calloc (n, sizeof  *Devices->addr);
	dev->max_addr = n;

	for (n = 0; n < dev->max_addr; ++n) {//fill out dev->addr table
		struct sockaddr_in *addr = (struct sockaddr_in *) (&dev->addr[n]);

		addr->sin_family = AF_INET;
		addr->sin_port = htons (Rport);
		memcpy (&addr->sin_addr,host->h_addr_list[n],host->h_length);
	}
}


