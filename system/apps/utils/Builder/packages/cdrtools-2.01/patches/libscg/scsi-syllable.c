/* @(#)scsi-syllable.c	1.0 05/02/01 Copyright 2005 Kristian Van Der Vliet */
#ifndef lint
static	char __sccsid[] =
	"@(#)scsi-syllable.c	1.0 05/02/01 Copyright 2005 Kristian Van Der Vliet";
#endif
/*
 *	Interface for the Syllable "SCSI" implementation.
 *
 *	Current versions of Syllable do not implement a complete SCSI layer, but
 *	the ATAPI portion of the ATA driver has a simple interface that can be used
 *	to send raw ATAPI packets to the device, much like the ATA driver for Linux.
 *
 *	This driver is an amalgamation of the scsi-beos and scsi-qnx drivers, both
 *	of which are nice and simple.  Because we don't have a real SCSI bus, scanning
 *	is not supported.
 */
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; see the file COPYING.  If not, write to the Free Software
 * Foundation, 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

LOCAL	char	_scg_trans_version[] = "scsi-syllable.c-1.0";	/* The version for this transport*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <atheos/types.h>
#include <atheos/cdrom.h>

#define	MAX_SCG		16	/* Max # of SCSI controllers */

struct scg_local {
	int		fd;
};

#define	scglocal(p)	((struct scg_local *)((p)->local))

/*
 * Return version information for the low level SCSI transport code.
 * This has been introduced to make it easier to trace down problems
 * in applications.
 */
LOCAL char *
scgo_version(scgp, what)
	SCSI	*scgp;
	int	what;
{
	if (scgp != (SCSI *)0) {
		switch (what) {

		case SCG_VERSION:
			return (_scg_trans_version);
		case SCG_AUTHOR:
			return "vanders";
		case SCG_SCCS_ID:
			return (__sccsid);
		}
	}
	return ((char *)0);
}

LOCAL int
scgo_help(scgp, f)
	SCSI	*scgp;
	FILE	*f;
{
	__scg_help(f, "ATA", "ATA Packet specific SCSI transport",
		"ATAPI:", "device", "/dev/disk/ata/cdc/raw", FALSE, TRUE);
	return (0);
}

LOCAL int
scgo_open(scgp, device)
	SCSI	*scgp;
	char	*device;
{
	int fd;

	if (device == NULL || *device == '\0') {
		errno = EINVAL;
		if (scgp->errstr)
			js_snprintf(scgp->errstr, SCSI_ERRSTR_SIZE,
				"'devname' must be specified on this OS");
		return (-1);
	}

	if (scgp->local == NULL) {
		scgp->local = malloc(sizeof (struct scg_local));
		if (scgp->local == NULL)
			return (0);
		scglocal(scgp)->fd = -1;
	}

	if (scglocal(scgp)->fd != -1)	/* multiple open? */
		return (1);

	if ((scglocal(scgp)->fd = open(device, O_RDONLY, 0)) < 0) {
		if (scgp->errstr)
			js_snprintf(scgp->errstr, SCSI_ERRSTR_SIZE,
				"Cannot open '%s'", device);
		return (-1);
	}

	scg_settarget(scgp, 0, 0, 0);

	return (1);
}

LOCAL int
scgo_close(scgp)
	SCSI	*scgp;
{
	if (scgp->local == NULL)
		return (-1);

	if (scglocal(scgp)->fd >= 0)
		close(scglocal(scgp)->fd);
	scglocal(scgp)->fd = -1;
	return (0);
}

LOCAL long
scgo_maxdma(scgp, amt)
	SCSI	*scgp;
	long	amt;
{
	return (256*1024);
}

LOCAL void *
scgo_getbuf(scgp, amt)
	SCSI	*scgp;
	long	amt;
{
	if (scgp->debug > 0) {
		js_fprintf((FILE *)scgp->errfile,
			"scgo_getbuf: %ld bytes\n", amt);
	}
	scgp->bufbase = malloc((size_t)(amt));
	return (scgp->bufbase);
}

LOCAL void
scgo_freebuf(scgp)
	SCSI	*scgp;
{
	if (scgp->bufbase)
		free(scgp->bufbase);
	scgp->bufbase = NULL;
}

LOCAL BOOL
scgo_havebus(scgp, busno)
	SCSI	*scgp;
	int	busno;
{
	return (FALSE);
}

LOCAL int
scgo_fileno(scgp, busno, tgt, tlun)
	SCSI	*scgp;
	int	busno;
	int	tgt;
	int	tlun;
{
	if (scgp->local == NULL)
		return (-1);

	return ((busno < 0 || busno >= MAX_SCG) ? -1 : scglocal(scgp)->fd);
}

LOCAL int
scgo_initiator_id(scgp)
	SCSI	*scgp;
{
	return (-1);
}

LOCAL int
scgo_isatapi(scgp)
	SCSI	*scgp;
{
	return (TRUE);
}

LOCAL int
scgo_reset(scgp, what)
	SCSI	*scgp;
	int	what;
{
	errno = EINVAL;
	return (-1);
}

LOCAL int
scgo_send(scgp)
	SCSI	*scgp;
{
	struct scg_cmd *sp = scgp->scmd;
	int	ret;
	cdrom_packet_cmd_s cmd;

	if (scgp->fd < 0) {
		sp->error = SCG_FATAL;
		return (-1);
	}

	/* Late run-time check */
	if( sp->cdb_len > 12 )
	{
		if (scgp->debug > 0)
			error("sp->cdb_len > 12!\n");
		sp->error = SCG_FATAL;
		sp->ux_errno = EIO;
		return (0);
	}

	/* initialize */
	fillbytes((caddr_t) & cmd, sizeof (cmd), '\0');
	fillbytes((caddr_t) & sp->u_sense.cmd_sense, sizeof (sp->u_sense.cmd_sense), '\0');

	memcpy(cmd.nCommand, &(sp->cdb), sp->cdb_len);
	cmd.nCommandLength = sp->cdb_len;
	cmd.pnData = (uint8*)sp->addr;
	cmd.nDataLength = sp->size;
	cmd.pnSense = (uint8*)sp->u_sense.cmd_sense;
	cmd.nSenseLength = sp->sense_len;
	cmd.nFlags = 0;

	if( sp->size > 0 )
	{
		if( sp->flags & SCG_RECV_DATA )
			cmd.nDirection = READ;
		else
			cmd.nDirection = WRITE;
	}
	else
		cmd.nDirection = NO_DATA;

	if (scgp->debug > 0) {
		error("SEND(%d): cmd %02x, cdb = %d, data = %d, sense = %d\n",
			scgp->fd, cmd.nCommand[0], cmd.nCommandLength,
			cmd.nDataLength, cmd.nSenseLength);
	}
	ret = ioctl(scgp->fd, CD_PACKET_COMMAND, &cmd, sizeof (cmd));
	if( ret < 0 )
		sp->ux_errno = geterrno();

	if (ret < 0 && scgp->debug > 4) {
		js_fprintf((FILE *) scgp->errfile,
			"ioctl(CD_PACKET_COMMAND) ret: %d\n", ret);
	}

	if( ret < 0 && cmd.nError )
	{
		sp->u_scb.cmd_scb[0] = ST_CHK_COND;
		if (scgp->debug > 0)
			error("result: scsi %02x\n", cmd.nError);

		switch (cmd.nError)
		{
			case SC_UNIT_ATTENTION:
			case SC_NOT_READY:
				sp->error = SCG_RETRYABLE;	/* may be BUS_BUSY */
				sp->u_scb.cmd_scb[0] |= ST_BUSY;
				break;
			case SC_ILLEGAL_REQUEST:
				break;
			default:
				break;
		}
	}
	else
		sp->u_scb.cmd_scb[0] = 0x00;

	return (0);
}
