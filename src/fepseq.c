/* vi:ts=4:sw=4
 *
 * General purpose Japanese FEP control routines for MS-DOS.
 * Original is Written by Junn Ohta (ohta@src.ricoh.co.jp, msa02563)
 *
 *	void fep_force_on(void)
 *		turn FEP on by its default "on" status.
 *	void fep_force_off(void)
 *		don't save the status of FEP and turn it off.
 *	int fep_get_mode(void)
 *		return the current status of FEP (0 = off).
 *
 */
#ifdef FEPCTRL

#include "vim.h"
#include "proto.h"

static int	fep_status = 0;
static int	keepmode = 0;

void fep_force_off()
{
	if (fep_status == 1)
	{
		if (T_FQ)
			outstr(T_FQ);
	}
	fep_status = 0;
}

void fep_force_on()
{
	if (fep_status == 0)
	{
		if (T_FB)
			outstr(T_FB);
	}
	fep_status = 1;
}

int fep_get_mode()
{
	return fep_status;
}

int fep_init()
{
	return 1;
}

void fep_term()
{
	;
}

void fep_on()
{
	if (keepmode)
		fep_force_on();
}

void fep_off()
{
	keepmode = fep_status;
	fep_force_off();
}

#endif	/* FEPCTRL */
