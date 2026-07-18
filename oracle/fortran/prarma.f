      SUBROUTINE prARMA(Fh)
      IMPLICIT NONE
c-----------------------------------------------------------------------
c     Prints out input file with regression, ARIMA specs
c-----------------------------------------------------------------------
      INCLUDE 'srslen.prm'
      INCLUDE 'model.prm'
      INCLUDE 'model.cmn'
      INCLUDE 'mdldat.cmn'
c-----------------------------------------------------------------------
      CHARACTER armopr*(4)
      INTEGER Fh,iflt,begopr,endopr,iopr,beglag,endlag,ilag
      DIMENSION armopr(2:3)
c-----------------------------------------------------------------------
      DATA armopr/'ar  ','ma  '/
c-----------------------------------------------------------------------
c     Write out the values
c Probably should only the differencing if it is different
c then the (1-B^sp)^d form.  This would be hard.
c-----------------------------------------------------------------------
      DO iflt=AR,MA
       begopr=Mdl(iflt-1)
       endopr=Mdl(iflt)-1
       IF(endopr.ge.begopr)THEN
        WRITE(fh,1070)armopr(iflt)
 1070   FORMAT('   ',a,'=(')
c     ------------------------------------------------------------------
        DO iopr=begopr,endopr
         beglag=Opr(iopr-1)
         endlag=Opr(iopr)-1
c     ------------------------------------------------------------------
         DO ilag=beglag,endlag
          IF(Arimaf(ilag))THEN
           WRITE(fh,1080)Arimap(ilag),'f'
 1080      FORMAT('    ',e24.10,a)
          ELSE
           WRITE(fh,1080)Arimap(ilag)
          END IF
         END DO
        END DO
        WRITE(fh,1090)
 1090   FORMAT('   )')
       END IF
      END DO
c     ------------------------------------------------------------------
      RETURN
      END
      