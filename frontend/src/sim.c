#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#include <grp.h>
#include <wiringPi.h>

#define OUTPUT_PIN	2
#define HIGH_TIME	300*1000
#define SLEEP_TIME	15000*1000

int init ();
int main_loop ();

int main (int argc, char *argv[])
{
	init ();
	main_loop ();	
	return 0;
}


int init ()
{
	/* setup wiringPi Library, NEED to be executed as root */
	wiringPiSetup();
	/*******************************************************/

	/* get back to non-privileged user *********************/
	struct passwd * pwd = getpwnam("emdc");
	struct group * grp = getgrnam("emdc");
	uid_t uid = pwd->pw_uid;
	gid_t gid = grp->gr_gid;
	setgid(gid);
	setuid(uid);
	/*******************************************************/

	pinMode (OUTPUT_PIN, OUTPUT);
	return 0;
}

int main_loop ()
{
	while (1)
	{
		printf (".");
                fflush (stdout);
		digitalWrite (OUTPUT_PIN, HIGH);
		usleep (HIGH_TIME);
		digitalWrite (OUTPUT_PIN, LOW);
		usleep (SLEEP_TIME - HIGH_TIME);
		printf (":");
		fflush (stdout);
	}
	return 0;
}

