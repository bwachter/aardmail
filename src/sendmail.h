/**
 * @file sendmail.h
 * @author Bernd Wachter <bwachter@lart.info>
 * @date 2005-2011
 */

#ifndef SENDMAIL_H
#define SENDMAIL_H

/// Constants used to remember which headers are present
enum AM_HEADERCHECKS {
  AM_HASFROM=1, /// From: header exists
  AM_HASTO  =2, /// To: header exists
  AM_HASDATE=4, /// Date: header exists
};

#endif
