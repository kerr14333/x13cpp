      SUBROUTINE setamx(Mdindx,Lseff,Locok,Inptok)
      IMPLICIT NONE
c     ------------------------------------------------------------------
c     Set Automatic model for pickmdl if file is not specified
c-----------------------------------------------------------------------
      INCLUDE 'stdio.i'
      INCLUDE 'units.cmn'
      INCLUDE 'error.cmn'
c-----------------------------------------------------------------------
      CHARACTER tmpdsn*(132)
      INTEGER Mdindx,sdiff,sma,nsar,nsdiff,nsma,ntmp
      LOGICAL Lseff,Locok,Inptok
c-----------------------------------------------------------------------
c     Set order of seasonal differencing and seasonal moving average
c-----------------------------------------------------------------------
      IF(Lseff)THEN
       sdiff=0
       sma=0
      ELSE
       sdiff=1
       sma=1
      END IF
c-----------------------------------------------------------------------
c     set model depending on index of model requested 
c-----------------------------------------------------------------------
      IF(Mdindx.eq.1)THEN
       nsar=0
       nsdiff=1
       nsma=1
      ELSE IF(Mdindx.eq.2)THEN
       nsar=0
       nsdiff=1
       nsma=2
      ELSE IF(Mdindx.eq.3)THEN
       nsar=2
       nsdiff=1
       nsma=0
      ELSE IF(Mdindx.eq.4)THEN
       nsar=0
       nsdiff=2
       nsma=2
      ELSE IF(Mdindx.eq.5)THEN
       nsar=2
       nsdiff=1
       nsma=2
      END IF
      CALL mdlset(nsar,nsdiff,nsma,0,sdiff,sma,Locok)
      Inptok=Inptok.and.Locok
c-----------------------------------------------------------------------
      IF((.not.Locok).or.Lfatal)THEN
       CALL mkmdsn(nsar,nsdiff,nsma,0,sdiff,sma,tmpdsn,ntmp)
       WRITE(STDERR,1010)
       WRITE(Mt2,1010)
      END IF
c-----------------------------------------------------------------------
 1010 FORMAT(/,' ERROR: Unable to set up ARIMA model ',a,' for pickmdl',
     &       /,'        automatic model selection procedure for the ',
     &         'reason(s)',/,'given above.')
c-----------------------------------------------------------------------
      RETURN
      END
