C     Last change:  BCM  23 Aug 2006   10:52 am
      SUBROUTINE prtdwr()
      IMPLICIT NONE
c-----------------------------------------------------------------------
      DOUBLE PRECISION ONE,ZERO
      PARAMETER(ONE=1.0D0,ZERO=0.0D0)
c-----------------------------------------------------------------------
      INCLUDE 'srslen.prm'
      INCLUDE 'model.prm'
      INCLUDE 'model.cmn'
      INCLUDE 'mdldat.cmn'
      INCLUDE 'arima.cmn'
      INCLUDE 'units.cmn'
c-----------------------------------------------------------------------
      CHARACTER tdstr*(PGRPCR),td2str*(PGRPCR),td3str*(PGRPCR),
     &          td4str*(PGRPCR)
      DOUBLE PRECISION sumb,sumcr,sumcr2,sumcr3,tdwrg,tdwcr,tdwcr2,
     &                 tdwcr3,addon
      INTEGER icol,imark,nrg,ncr,ncr2,ncr3,ncol,ncol2,ncol3,ncol4,igrp,
     &        begcol,endcol,Tbcode
      DIMENSION tdwrg(6),tdwcr(6),tdwcr2(6),tdwcr3(6)
c-----------------------------------------------------------------------
      LOGICAL dpeq
      EXTERNAL dpeq
c-----------------------------------------------------------------------
      IF(Nb.eq.0)RETURN
      nrg = 0
      ncr = 0
      ncr2 = 0
      ncr3 = 0
      sumb = ZERO
      sumcr = ZERO
      sumcr2 = ZERO
      sumcr3 = ZERO
      addon = ZERO
      IF(dpeq(Lam,ZERO))addon=ONE
c-----------------------------------------------------------------------
      DO igrp=1,Ngrp
       begcol=Grp(igrp-1)
       endcol=Grp(igrp)-1
       imark=Rgvrtp(begcol)
       IF(imark.eq.PRGTTD)THEN
        DO icol=begcol,endcol
         nrg=nrg+1
         tdwrg(nrg)=B(icol)+addon
         sumb = sumb - B(icol)
         IF(nrg.eq.1)CALL getstr(Grpttl,Grpptr,Ngrp,igrp,tdstr,ncol)
        END DO
       ELSE IF(imark.eq.PRRTTD)THEN
        DO icol=begcol,endcol
         ncr=ncr+1
         tdwcr(ncr)=B(icol)+addon
         sumcr = sumcr - B(icol)
         IF(ncr.eq.1)CALL getstr(Grpttl,Grpptr,Ngrp,igrp,td2str,ncol2)
        END DO
       ELSE IF(imark.EQ.PRATTD)THEN
        DO icol=begcol,endcol
         ncr2=ncr2+1
         tdwcr2(ncr2)=B(icol)+addon
         sumcr2 = sumcr2 - B(icol)
         IF(ncr2.eq.1)CALL getstr(Grpttl,Grpptr,Ngrp,igrp,td3str,ncol3)
        END DO
       ELSE IF(imark.EQ.PRGUTD)THEN
        DO icol=begcol,endcol
         ncr3=ncr3+1
         tdwcr3(ncr3)=B(icol)+addon
         sumcr3 = sumcr3 - B(icol)
         IF(ncr3.eq.1)CALL getstr(Grpttl,Grpptr,Ngrp,igrp,td4str,ncol4)
        END DO
       END IF
      END DO
c     ------------------------------------------------------------------
      IF(nrg.eq.0.and.ncr.eq.0.and.ncr2.eq.0)RETURN
c     ------------------------------------------------------------------
      WRITE(Mt1,1000)' '
      WRITE(Mt1,1000)'Regression Trading Day Weights'
      WRITE(Mt1,1010)
      WRITE(Mt1,1000)
     & '---------------------------------------------------------------'
      WRITE(Mt1,1020)
      WRITE(Mt1,1000)
     & '---------------------------------------------------------------'
      IF(nrg.gt.0)THEN
       sumb = sumb + ONE
       WRITE(Mt1,1000)tdstr(1:ncol)
       IF(dpeq(Lam,ZERO))THEN
        WRITE(Mt1,1030)(tdwrg(icol), icol = 1, 6), sumb
       ELSE
        WRITE(Mt1,1040)(tdwrg(icol), icol = 1, 6), sumb
       END IF
      END IF
      IF(ncr.gt.0)THEN
       sumcr = sumcr + ONE
       WRITE(Mt1,1000)td2str(1:ncol2)
       IF(dpeq(Lam,ZERO))THEN
        WRITE(Mt1,1030)(tdwcr(icol), icol = 1, 6), sumcr
       ELSE
        WRITE(Mt1,1040)(tdwcr(icol), icol = 1, 6), sumcr
       END IF
      END IF
      IF(ncr2.gt.0)THEN
       sumb = sumb + ONE
       sumcr2 = sumcr2 + ONE
       WRITE(Mt1,1000)td3str(1:ncol3)
       IF(dpeq(Lam,ZERO))THEN
        WRITE(Mt1,1030)(tdwcr2(icol), icol = 1, 6), sumcr2
       ELSE
        WRITE(Mt1,1040)(tdwcr2(icol), icol = 1, 6), sumcr2
       END IF
      END IF
      IF(ncr3.gt.0)THEN
       sumcr3 = sumcr3 + ONE
       WRITE(Mt1,1000)td4str(1:ncol4)
       IF(dpeq(Lam,ZERO))THEN
        WRITE(Mt1,1030)(tdwcr3(icol), icol = 1, 6), sumcr3
       ELSE
        WRITE(Mt1,1040)(tdwcr2(icol), icol = 1, 6), sumcr3
       END IF
      END IF
      WRITE(Mt1,1000)
     & '---------------------------------------------------------------'
c-----------------------------------------------------------------------
 1000 FORMAT(1x,a)
 1010 FORMAT(5x,'(Regression Trading Day Coefficients expressed as ',/,
     &       6x,'X-11 Trading Day Weights)',/)
 1020 FORMAT('            Mon     Tue     Wed     Thu     Fri     Sat',
     &       '    *Sun (derived)')
 1030 FORMAT(8x,7f8.4)
 1040 FORMAT(8x,7f8.1)
c-----------------------------------------------------------------------
      RETURN
      END

