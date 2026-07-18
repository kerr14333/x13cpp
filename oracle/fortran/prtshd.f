C     Last change:Jan. 2021, change blnk to lenghth 79
C     previous change:  BCM  21 Apr 98    2:17 pm
**==prtshd.f    processed by SPAG 4.03F  at 09:52 on  1 Mar 1994
      SUBROUTINE prtshd(Ttlhdr,Begdat,Sp,Nobs,Locpag)
      IMPLICIT NONE
c-----------------------------------------------------------------------
c     Prints the header information for a time series.
c-----------------------------------------------------------------------
      INCLUDE 'units.cmn'
      INCLUDE 'error.cmn'
      INCLUDE 'title.cmn'
c     ------------------------------------------------------------------
      INTEGER PSRSCR
      PARAMETER(PSRSCR=79)
c     ------------------------------------------------------------------
      LOGICAL Locpag
      CHARACTER Ttlhdr*(*),bdtstr*(10),blnk*(PSRSCR),edtstr*(10)
      INTEGER Begdat,idate,nchr1,nchr2,Nobs,Sp
      DIMENSION Begdat(2),idate(2)
c     ------------------------------------------------------------------
      DATA blnk/
     &'                                                                 
     &              '/
c     ------------------------------------------------------------------
      IF(Lpage.and.Locpag)THEN
       WRITE(Mt1,Ttlfmt)Newpg,Title(1:Ntitle),Kpage,Serno(1:Nser)
       Kpage=Kpage+1
      END IF
      CALL addate(Begdat,Sp,Nobs-1,idate)
      CALL wrtdat(Begdat,Sp,bdtstr,nchr1)
      IF(.not.Lfatal)CALL wrtdat(idate,Sp,edtstr,nchr2)
      IF(Lfatal)RETURN
      IF(len(Ttlhdr).gt.0)WRITE(Mt1,1020)Ttlhdr
 1020 FORMAT(/,' ',a)
      IF(Nobs.gt.0)THEN
       WRITE(Mt1,1030)blnk(1:17-nchr1-nchr2),bdtstr(1:nchr1),
     &                edtstr(1:nchr2),Nobs
 1030  FORMAT('  From ',a,a,' to ',a,/,'  Observations     ',i6)
      END IF
c     ------------------------------------------------------------------
      RETURN
c     ------------------------------------------------------------------
      END
