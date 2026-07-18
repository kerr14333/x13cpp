C     Last change:  BCM   8 Aug 2011    9:44 am
**==savacf.f    processed by SPAG 4.03F  at 10:31 on 29 Jul 1994
      SUBROUTINE savotl(Lsumm,Lsvlog,Gudrun,Lidotl)
c-----------------------------------------------------------------------
c     Print out entries for diagnostic and log files related to outlier
c     regressors
c-----------------------------------------------------------------------
      IMPLICIT NONE
c-----------------------------------------------------------------------
      DOUBLE PRECISION ZERO,TWO
      PARAMETER(ZERO=0D0,TWO=2D0)
c-----------------------------------------------------------------------
      INCLUDE 'srslen.prm'
      INCLUDE 'model.prm'
      INCLUDE 'model.cmn'
      INCLUDE 'mdldat.cmn'
      INCLUDE 'usrreg.cmn'
      INCLUDE 'units.cmn'
      INCLUDE 'error.cmn'
c     ------------------------------------------------------------------
      INTEGER Lsumm,i1,i2,iauto,iao,ils,itc,iso,iramp,itls,iuser,nchr,
     &        nb2,nelt,nfix,iall
      LOGICAL Gudrun,Lsvlog,Lidotl
      CHARACTER icoltl*(PCOLCR)
      DOUBLE PRECISION xpxinv,tmp,rmse,seb
      DIMENSION xpxinv(PXPX),tmp(2)
c-----------------------------------------------------------------------
      DOUBLE PRECISION dpmpar
      LOGICAL dpeq
      EXTERNAL dpmpar,dpeq
c-----------------------------------------------------------------------
      IF(.not.((Lsumm.gt.0.or.(Lsvlog.and.Lidotl)).and.gudrun))RETURN
c-----------------------------------------------------------------------
      iauto=0
      iall=0
      iao=0
      ils=0
      itc=0
      iso=0
      iramp=0
      itls=0
      iuser=0
c-----------------------------------------------------------------------
c     if saving info to log, initialize variables needed to generate
c     t-statistic for outlier regressors
c-----------------------------------------------------------------------
      IF(Lsvlog)THEN
c     ------------------------------------------------------------------
c     Generate number of unfixed regressors
c     ------------------------------------------------------------------
       nb2=Nb
       IF(Iregfx.ge.2)THEN
        DO i1=1,Nb
         IF(Regfx(i1))nb2=nb2-1
        END DO
       END IF
c-----------------------------------------------------------------------
c     Get the root mean square error and X'X inverse.
c-----------------------------------------------------------------------
       IF(nb2.gt.0)THEN
c        nelt=Ncxy*(Ncxy+1)/2
        nelt=(nb2+1)*(nb2+2)/2
c-----------------------------------------------------------------------
        IF(Var.gt.TWO*dpmpar(1))THEN
         rmse=sqrt(Var)
         CALL copy(Chlxpx,nelt,1,xpxinv)
         CALL dppdi(xpxinv,nb2,tmp,1)
c         CALL dppdi(xpxinv,Nb,tmp,1)
c-----------------------------------------------------------------------
        ELSE
         rmse=ZERO
        END IF
       ELSE
        rmse=ZERO
       END IF
       nfix=0
      END IF
c-----------------------------------------------------------------------
      IF(Nb.gt.0)THEN
       DO i1=1,Nb
        IF(Rgvrtp(i1).eq.PRGTAA.or.Rgvrtp(i1).eq.PRGTAL.or.
*     &     Rgvrtp(i1).eq.PRGTAT.or.Rgvrtp(i1).eq.PRGTAS)THEN
     &     Rgvrtp(i1).eq.PRGTAT)THEN
         iauto=iauto+1
         IF(Lsvlog)THEN
          IF(iauto.eq.1)THEN
           WRITE(Ng,1060)' '
           WRITE(Ng,1060)'  Outliers identifed in this run:'
          END IF
c-----------------------------------------------------------------------
c    Compute standard error of regressor
c-----------------------------------------------------------------------
          i2=i1-nfix
          seb=sqrt(xpxinv(i2*(i2+1)/2))*rmse
c-----------------------------------------------------------------------
c     Print out regressor with t statistic
c-----------------------------------------------------------------------
          CALL getstr(Colttl,Colptr,Nb,i1,icoltl,nchr)
          IF(Lfatal)RETURN
          IF(dpeq(seb,ZERO))THEN
           WRITE(Ng,1060)'     ',icoltl(1:nchr)
          ELSE
           WRITE(Ng,1050)icoltl(1:nchr),B(i1)/seb
          END IF
         END IF
        ELSE IF(Lsvlog)THEN
         IF(Regfx(i1))nfix=nfix+1
        END IF
        IF(Rgvrtp(i1).eq.PRGTAA.or.Rgvrtp(i1).eq.PRGTAO.or.
     &     Rgvrtp(i1).eq.PRGUAO)THEN
         iao=iao+1
         iall=iall+1
        END IF
        IF(Rgvrtp(i1).eq.PRGTAL.or.Rgvrtp(i1).eq.PRGTLS.or.
     &     Rgvrtp(i1).eq.PRGULS)THEN
         ils=ils+1
         iall=iall+1
        END IF
        IF(Rgvrtp(i1).eq.PRGTAT.or.Rgvrtp(i1).eq.PRGTTC)THEN
         itc=itc+1
         iall=iall+1
        END IF
*        IF(Rgvrtp(i1).eq.PRGTAS.or.Rgvrtp(i1).eq.PRGTSO)iso=iso+1
        IF(Rgvrtp(i1).eq.PRGTSO.or.Rgvrtp(i1).eq.PRGUSO)THEN
         iso=iso+1
         iall=iall+1
        END IF
        IF(Rgvrtp(i1).eq.PRGTRP.or.Rgvrtp(i1).eq.PRGTQD.or.
     &     Rgvrtp(i1).eq.PRGTQI)THEN
         iramp=iramp+1
         iall=iall+1
        END IF
        IF(Rgvrtp(i1).eq.PRGTTL)THEN
         itls=itls+1
         iall=iall+1
        END IF
        IF(Ncusrx.gt.0)THEN
         IF(Rgvrtp(i1).eq.PRGUAO.or.Rgvrtp(i1).eq.PRGULS.or.
     &      Rgvrtp(i1).eq.PRGUSO)iuser=iuser+1
        END IF
       END DO
      END IF
      IF(Lsumm.gt.0)THEN
       WRITE(Nform,1080)'outlier.ao: ',iao
       WRITE(Nform,1080)'outlier.ls: ',ils
       WRITE(Nform,1080)'outlier.tc: ',itc
       WRITE(Nform,1080)'outlier.so: ',iso
       WRITE(Nform,1080)'outlier.rp: ',iramp
       WRITE(Nform,1080)'outlier.tls: ',itls
       IF(Ncusrx.gt.0)WRITE(Nform,1080)'outlier.user: ',iuser
       WRITE(Nform,1080)'outlier.total: ',iall
       IF(lidotl)WRITE(Nform,1080)'autoout: ',iauto
      END IF
      IF(Lsvlog)THEN
       WRITE(Ng,1060)'   '
       IF(iauto.eq.0)THEN
        WRITE(Ng,1060)'   ','No outliers identified'
       ELSE
        WRITE(Ng,1070)'   Total number of outliers identified: ',iauto
       END IF
       WRITE(Ng,1060)'   '
      END IF
c     ------------------------------------------------------------------
 1050 FORMAT(5x,a,' (t=',f10.2,')')
 1060 FORMAT(a:,a)
 1070 FORMAT(a,i6)
 1080 FORMAT(a,i2)
c     ------------------------------------------------------------------
      RETURN
      END
