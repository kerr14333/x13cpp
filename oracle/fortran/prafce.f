C     Last change:  BCM  23 Sep 1998   10:14 am
      SUBROUTINE prafce(Mt1,Mape,Outfer,Lfcst)
      IMPLICIT NONE
c-----------------------------------------------------------------------
c     Print out average fcst std. error for the last three years
c-----------------------------------------------------------------------
      INCLUDE 'title.cmn'
c-----------------------------------------------------------------------
      CHARACTER cfcst*(9)
      DOUBLE PRECISION Mape
      INTEGER i,Mt1
      LOGICAL Lfcst,Outfer
      DIMENSION Mape(4)
c-----------------------------------------------------------------------
      IF(Lfcst)THEN
       cfcst='forecasts'
      ELSE
       cfcst='backcasts'
      END IF
c-----------------------------------------------------------------------
      IF(.not.Lcmpaq)WRITE(Mt1,'()')
      IF(Outfer)THEN
       WRITE(Mt1,1010)'out-of-sample',cfcst
      ELSE
       WRITE(Mt1,1010)'within-sample',cfcst
      END IF
 1010 FORMAT(' Average absolute percentage error in ',a,' ',a,':')
c-----------------------------------------------------------------------
      IF(Lcmpaq)THEN
       WRITE(Mt1,1020)(Mape(i),i=1,4)
      ELSE
       WRITE(Mt1,1030)(Mape(i),i=1,4)
      END IF
 1020 FORMAT('   Last year: ',f6.2,'      Last-1 year: ',f6.2,
     &      '     Last-2 year: ',f6.2,/,'   Last three years:  ',f6.2,/)
 1030 FORMAT('  Last year: ',f6.2,'      Last-1 year: ',f6.2,
     &       '     Last-2 year: ',f6.2,/,'  Last three years:  ',f6.2,/)
c-----------------------------------------------------------------------
      RETURN
      END
