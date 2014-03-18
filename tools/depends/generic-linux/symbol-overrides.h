/*
 *      Copyright (C) 2014 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#ifndef __XBMC_SYMBOL_OVERRIDES_H
#define __XBMC_SYMBOL_OVERRIDES_H

#define __XBMC_TO_STRING_(x) #x
#define __XBMC_TO_STRING(x) __XBMC_TO_STRING_(x)

#define __XBMC_GLIBC_COMPAT_SYMBOL_VERSION_BASE   GLIBC_2.0
#define __XBMC_GLIBC_COMPAT_SYMBOL_VERSION_X86_64 GLIBC_2.2.5

#ifdef __amd64__
#  define __XBMC_COMPAT_SYMBOL_VERSION __XBMC_GLIBC_COMPAT_SYMBOL_VERSION_X86_64
#else
#  define __XBMC_COMPAT_SYMBOL_VERSION __XBMC_GLIBC_COMPAT_SYMBOL_VERSION_BASE
#endif

#ifdef __ASSEMBLER__
#  define __XBMC_COMPAT_SYMBOL_(sym_new, sym_old, ver_old) .symver sym_new ## , ## sym_old ## @ ## ver_old
#else
#  define __XBMC_COMPAT_SYMBOL_(sym_new, sym_old, ver_old) __asm__(".symver " #sym_new "," #sym_old "@" __XBMC_TO_STRING(ver_old) );
#endif

#define __XBMC_GLIBC_COMPAT_SYMBOL(sym) __XBMC_COMPAT_SYMBOL_(sym, sym, __XBMC_COMPAT_SYMBOL_VERSION)
#define __XBMC_GLIBC_COMPAT_SYMBOL_REDIRECT(sym_new, sym) __XBMC_COMPAT_SYMBOL_(sym_new, sym, __XBMC_COMPAT_SYMBOL_VERSION)

/* Use e.g. "objdump -T file | grep $VERSION" to find out the symbol names */

/* does not exist pre-2.12 (this just makes configure checks fail) */
__XBMC_GLIBC_COMPAT_SYMBOL(pthread_setname_np)

/* use pre-2.14 version of memcpy (memmove semantics; we use it just because
 * it is always available) */
__XBMC_GLIBC_COMPAT_SYMBOL(memcpy)

/* finite versions did not exist on pre-2.15 */
__XBMC_GLIBC_COMPAT_SYMBOL_REDIRECT(__acosf_finite, acosf)
__XBMC_GLIBC_COMPAT_SYMBOL_REDIRECT(__exp_finite, exp)
__XBMC_GLIBC_COMPAT_SYMBOL_REDIRECT(__pow_finite, pow)
__XBMC_GLIBC_COMPAT_SYMBOL_REDIRECT(__log_finite, log)

/* use librt version of clock_gettime (it did not exist in pre-2.17 glibc) */
__XBMC_GLIBC_COMPAT_SYMBOL(clock_gettime)

/*
 * Avoid dependency on __fdelt_chk from <sys/select.h> when _FORTIFY_SOURCE
 * is enabled. This is a bit overkill, maybe we could provide our own version
 * __fdelt_chk instead, it is a oneliner (d / __NFDBITS).
 */
#undef _FORTIFY_SOURCE

#endif
