/* vi:ts=4:sw=4
 *
 * General purpose Japanese IME control routines for BOW (BSD on Windows).
 * Original is Written by Junn Ohta (ohta@src.ricoh.co.jp)
 *                        Hiroshi Oota (lda01607@niftyserve.or.jp)
 * BOW IME Control routines written by K.Tsuchida (ken_t@st.rim.or.jp).
 *
 *	int fep_init(void)
 *		checks FEP and turn it off, returns FEP type.
 *	void fep_term(void)
 *		restore the status of FEP saved by fep_init().
 *	void fep_on(void)
 *		restore the status of FEP saved by fep_off().
 *	void fep_off(void)
 *		save the status of FEP and turn it off.
 *	void fep_force_on(void)
 *		turn FEP on by its default "on" status.
 *	void fep_force_off(void)
 *		don't save the status of FEP and turn it off.
 *	int fep_get_mode(void)
 *		return the current status of FEP (0 = off).
 *
 */
#ifdef FEPCTRL

#include <sys/types.h>
#include <sys/termios.h>
#include <sys/ioctl.h>

static int	init = 0;

void fep_force_off()
{
	struct termios termios;
	int n;

	if (!init)
		return;
	if (ioctl(0, TIOCGETA, &termios))
		return;
	n =  termios.c_cflag;
	n &= ~CLOCAL;
	if (n != termios.c_cflag) {
		termios.c_cflag = n;
		ioctl(0, TIOCSETA, &termios);
	}
}

void fep_force_on()
{
	struct termios termios;
	int n;

	if (!init)
		return;
	if (ioctl(0, TIOCGETA, &termios))
		return;
	n =  termios.c_cflag;
	n |= CLOCAL;
	if (n != termios.c_cflag) {
		termios.c_cflag = n;
		ioctl(0, TIOCSETA, &termios);
	}
}

int fep_get_mode()
{
	struct termios termios;

	if (!init)
		return 0;
	if (ioctl(0, TIOCGETA, &termios))
		return 0;
	if (termios.c_cflag & CLOCAL)
		return 1;
	return 0;
}

int fep_init()
{
	init = 1;
	return 1;
}

void fep_term()
{
	init = 0;
}

static int	keepmode = 0;

void fep_on()
{
	if (!init)
		return;
	if (keepmode)
		fep_force_on();
}

void fep_off()
{
	if (!init)
		return;
	keepmode = fep_get_mode();
	fep_force_off();
}

#endif	/* FEPCTRL */
