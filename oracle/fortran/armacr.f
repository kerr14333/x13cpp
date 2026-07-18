C     Last change:  BCM  24 Nov 97   12:26 pm
      SUBROUTINE armacr
      IMPLICIT NONE
c-----------------------------------------------------------------------
c     Calculates and print the correlation matrix from (X'X)^-1
c-----------------------------------------------------------------------
      INCLUDE 'srslen.prm'
      INCLUDE 'model.prm'
      INCLUDE 'model.cmn'
      INCLUDE 'mdldat.cmn'
      INCLUDE 'units.cmn'
      INCLUDE 'error.cmn'
c     ------------------------------------------------------------------
      INTEGER OPRS
      PARAMETER(OPRS=2)
c     ------------------------------------------------------------------
      CHARACTER cfix*7,tmpttl*(POPRCR)
      INTEGER beglag,begopr,endlag,endopr,icol,iestpm,iflt,ilag,iopr,j,
     &        ntmpcr
c     ------------------------------------------------------------------
      IF(Nestpm.le.1)RETURN
      iestpm=0
c     ------------------------------------------------------------------
      WRITE(Mt1,1010)(icol,icol=1,Nestpm)
 1010 FORMAT(/,' ARMA Parameter Correlation matrix',/,'  Parameter',
     &       (:t15,10I6))
      WRITE(Mt1,1020)('-',icol=1,12+6*min(Nestpm,10))
 1020 FORMAT('  ',(78a))
c     ------------------------------------------------------------------
      DO iflt=AR,MA
       begopr=Mdl(iflt-1)
       endopr=Mdl(iflt)-1
c     ------------------------------------------------------------------
       DO iopr=begopr,endopr
        beglag=Opr(iopr-1)
        endlag=Opr(iopr)-1
c     ------------------------------------------------------------------
        CALL isfixd(OPRS,Arimaf,beglag,endlag,cfix)
        IF(cfix.eq.'       ')THEN
         CALL getstr(Oprttl,Oprptr,Noprtl,iopr,tmpttl,ntmpcr)
         IF(Lfatal)RETURN
         WRITE(Mt1,1030)tmpttl(1:ntmpcr)
 1030    FORMAT('  ',a,t39,a)
c     ------------------------------------------------------------------
         DO ilag=beglag,endlag
          IF(.not.Arimaf(ilag))THEN
           iestpm=iestpm+1
           WRITE(Mt1,1040)Arimal(ilag),
     &                    (Armacm(iestpm,j)/sqrt(Armacm(j,j)
     &                    *Armacm(iestpm,iestpm)),j=1,iestpm)
 1040      FORMAT('   Lag',i3,5x,10F6.2,(:/,t14,10F6.2))
          END IF
         END DO
        END IF
       END DO
      END DO
c     ------------------------------------------------------------------
      RETURN
      END

