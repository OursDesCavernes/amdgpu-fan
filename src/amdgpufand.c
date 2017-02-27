#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <fcntl.h>
#include <stdlib.h>
#include <time.h>

#define MAX_CARDS 8
#define MAX_TABLE_SIZE 10
#define MIN_PCT 0
#define HYSTERESIS 6

struct card {
	int pwm;
	int pwm_min;
	int pwm_max;
	int pwmfd;
	int temp;
	int tempfd;
	int ready;
	char name[1024];
	int table[MAX_TABLE_SIZE][2];
	int temp_hist[HYSTERESIS];
	int hpct;};

typedef struct card card;

card cards[MAX_CARDS];

char *config_file;

int run = true;

int data_init()
{
	int i,j;
	for(i=0;i<MAX_CARDS;i++)
	{
		cards[i].pwm=-1;
		cards[i].pwmfd=-1;
		cards[i].temp=0;
		cards[i].tempfd=-1;
		cards[i].ready=false;
		for(j=0;j<MAX_TABLE_SIZE;j++)
		{
			cards[i].table[j][0]=0;
			cards[i].table[j][0]=100;
		}
		//temporary static table
		cards[i].table[0][0] = 40;
		cards[i].table[0][1] = 0;
		cards[i].table[1][0] = 45;
		cards[i].table[1][1] = 15;
		cards[i].table[2][0] = 50;
		cards[i].table[2][1] = 20;
		cards[i].table[3][0] = 55;
		cards[i].table[3][1] = 30;
		cards[i].table[4][0] = 60;
		cards[i].table[4][1] = 50;
		cards[i].table[5][0] = 70;
		cards[i].table[5][1] = 80;
		cards[i].table[6][0] = 80;
		cards[i].table[6][1] = 100;
	}
	for(i=0;i<HYSTERESIS;i++)
		cards[i].temp_hist[i] = 0;
	
	if(MIN_PCT<0||MIN_PCT>100)
		exit(-1);
	return 0;
}

int get_temp(int n)
{
	off_t offset;
	char buf[6];
	int temp;
	ssize_t res;
	if(cards[n].ready == false)
		return -1;
	offset = lseek(cards[n].tempfd, 0, SEEK_SET);
	res = read(cards[n].tempfd, buf,6);
	temp = atoi(buf);
	cards[n].temp = temp / 1000;
//	printf("Card %i: temp = %i\n", n, cards[n].temp);
}

int get_pwm(int n)
{
	off_t offset;
	char buf[5];
	int pwm;
	ssize_t res;
	if(cards[n].ready == false)
		return -1;
	offset = lseek(cards[n].pwmfd, 0, SEEK_SET);
	res = read(cards[n].pwmfd, buf,5);
	pwm = atoi(buf);
	if(pwm < cards[n].pwm_min || pwm > cards[n].pwm_max)
	{
		printf("error reading pwm for card %i\n", n);
		cards[n].ready = false;
	}
	cards[n].pwm = pwm;
//	printf("Card %i: pwm = %i\n", n, cards[n].pwm);
	return 0;
}

int get_pct(int n)
{
	int pct;
	pct = (cards[n].pwm - cards[n].pwm_min) * 100 / ( cards[n].pwm_max - cards[n].pwm_min );
	return pct;
}

int set_pwm(int n, int pct)
{
	off_t offset;
	char buf[5];
	int pwm,len;
	ssize_t res;
	if(cards[n].ready == false)
		return -1;
	if( pct<MIN_PCT || pct>100 )
	{
		printf("Error: Bad pct: %i\n",pct);
		return -1;
	}
	offset = lseek(cards[n].pwmfd, 0, SEEK_SET);
	pwm = cards[n].pwm_min + (cards[n].pwm_max - cards[n].pwm_min) * pct / 100;
//	printf("Set pwm: %i\n",pwm);
	sprintf(buf, "%i%n",pwm,&len);
	res = write(cards[n].pwmfd,buf,len);

	return 0;
}

int calc_pct(int n)
{
	int pct = -1,hpct, i;
	get_temp(n);
	for(i=0;i<MAX_TABLE_SIZE;i++)
	{
		if(cards[n].temp < cards[n].table[i][0])
		{
			pct = cards[n].table[i][1];
			break;
		}
		else if(cards[n].temp > cards[n].table[i][0] && i + 1 == MAX_TABLE_SIZE)
		{
			pct = cards[n].table[i][1];
			break;
		}
		else if(cards[n].temp > cards[n].table[i][0] && cards[n].table[i][0] >= cards[n].table[i+1][0])
		{
			pct = cards[i].table[i][1];
			break;
		}
		else if(cards[n].temp >= cards[n].table[i][0] && cards[n].temp < cards[n].table[i+1][0])
		{
			pct = cards[n].table[i][1] + ((cards[n].table[i+1][1] - cards[n].table[i][1]) * (cards[n].temp - cards[i].table[n][0]) / (cards[n].table[i+1][0] - cards[n].table[i][0]));
			break;
		}
	}
	hpct = pct;
	for(i=0;i<HYSTERESIS - 1;i++)
	{
		if(cards[n].temp_hist[i] > hpct)
			hpct = cards[n].temp_hist[i];
		cards[n].temp_hist[i+1] = cards[n].temp_hist[i];
	}
	if(hpct + 5 < cards[n].hpct)
		hpct = cards[n].hpct - 5;
	cards[n].hpct = hpct;
	cards[n].temp_hist[0] = pct;	
	set_pwm(n,hpct);
}

int select_cards(const struct dirent *drm)
{
	if(strncmp(drm->d_name,"card",4)!=0 || drm->d_name[4] < '0' || drm->d_name[4] > '9' || drm->d_name[5] != '\0')
		return 0;
	return 1;
}

int select_hwmon(const struct dirent *hwmon)
{
	if(strncmp(hwmon->d_name,"hwmon",5)!=0 || hwmon->d_name[5] < '0' || hwmon->d_name[5] > '9' || hwmon->d_name[6] != '\0')
		return 0;
	return 1;
}

int isamdgpu(char *path)
{
	char buffer[255],buf[10];
	int fd,res=-1;
	sprintf(buffer, "%s/name",path);

	fd = open(buffer,O_RDONLY);
	if(fd<0)
		return -1;
	
	if(read(fd, buf,10)>0)
		res = strncmp(buf,"amdgpu",6);
	close(fd);

	return res;
}

int probe_cards()
{
	int cardnum = 0, card = 0;
	int i,j;
	struct dirent **epsc;
	cardnum = scandir ("/sys/class/hwmon", &epsc, select_hwmon, alphasort);
	for(i=0;i<cardnum;i++)
	{
		int fd;
		char pathh[255];
		sprintf(pathh, "/sys/class/hwmon/%s",epsc[i]->d_name);
		if(isamdgpu(pathh) ==0 && card<MAX_CARDS )
		{
			char buffer[255],buf[6];
			sprintf(buffer, "/sys/class/hwmon/%s/pwm1",epsc[i]->d_name);
			cards[card].pwmfd = open(buffer,O_RDWR);
			if(cards[card].pwmfd<0)
			{
				printf("Error %i opening /sys/class/hwmon/%s/pwm1\n",cards[card].pwmfd, epsc[i]->d_name);
				free(epsc[j]);
				continue;
			}
			sprintf(buffer, "/sys/class/hwmon/%s/temp1_input",epsc[i]->d_name);
			cards[card].tempfd = open(buffer,O_RDONLY);
			if(cards[card].tempfd<0)
			{
				printf("Error %i opening /sys/class/hwmon/%s/temp1_input",cards[card].tempfd, epsc[i]->d_name);
				close(cards[card].pwmfd);
				free(epsc[j]);
				continue;
			}

			sprintf(buffer, "/sys/class/hwmon/%s/pwm1_min",epsc[i]->d_name);
			fd = open(buffer,O_RDONLY);
			if(read(fd, buf,6)<0)
			{
				close(fd);
				close(cards[card].pwmfd);
				close(cards[card].tempfd);
				free(epsc[j]);
				continue;
			}
			cards[card].pwm_min = atoi(buf);
			close(fd);

			sprintf(buffer, "/sys/class/hwmon/%s/pwm1_max",epsc[i]->d_name);
			fd = open(buffer,O_RDONLY);
			if(read(fd, buf,6)<0)
			{
				close(fd);
				close(cards[card].pwmfd);
				close(cards[card].tempfd);
				free(epsc[j]);
				continue;
			}
			cards[card].pwm_max = atoi(buf);
			close(fd);

			cards[card].ready = true;
			get_temp(card);
			get_pwm(card);
			card++;
		}
		free(epsc[i]);
	}
	if(cardnum > 0)
		free(epsc);
}

int read_config()
{
	return 0;
}

int loop()
{
	int res;
	struct timespec request,remain;
	request.tv_sec = 1;
	request.tv_nsec = 0;
	while(run)
	{
		calc_pct(0);
		res = clock_nanosleep(CLOCK_REALTIME, 0, &request, &remain);
		//run=false;
	}
}

int main(int argc, char** argv, char** envv)
{
	data_init();
	config_file = "/etc/amdgpufan/amdgpufand.conf";
	read_config();
	probe_cards();
	loop();
	return 0;
}


	
