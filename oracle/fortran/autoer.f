      SUBROUTINE autoer(Info)
      IMPLICIT NONE
c-----------------------------------------------------------------------
c   Check to see if ARIMA model estimation warnings are present.
c-----------------------------------------------------------------------
      LOGICAL T,F
      PARAMETER(T=.true.,F=.false.)
c     ------------------------------------------------------------------
      INCLUDE 'stdio.i'
      INCLUDE 'srslen.prm'
      INCLUDE 'model.prm'
      INCLUDE 'units.cmn'
c-----------------------------------------------------------------------
      INTEGER info
c-----------------------------------------------------------------------
      IF(info.eq.PINVER.or.info.eq.PGPGER.or.info.eq.PACFER.or.
     &   info.eq.PVWPER)THEN
       CALL writln('Model estimation warnings encountered during '//
     &             'automatic model identification.',Mt1,Mt2,T)
       CALL writln('Program will cease execution; warning message '//
     &             'given below.',Mt1,Mt2,F)
       WRITE(STDERR,*)' Model estimation warnings encountered during ',
     &                'automatic model identification.'
      ELSE
       RETURN
      END IF
c-----------------------------------------------------------------------
      IF(info.eq.PINVER)THEN
       CALL writln('ARMA roots inside the unit circle.',Mt1,Mt2,T)
       CALL abend()
c     ------------------------------------------------------------------
      ELSE IF(info.eq.PGPGER)THEN
       CALL writln('Problem with MA parameter estimation.  '//PRGNAM//
     &               ' can''t',Mt1,Mt2,T)
       CALL writln('          invert the G''G matrix. Try a '//
     &               'simpler ARIMA model without',Mt1,Mt2,F)
       CALL writln('          parameter constraints. Please send '//
     &               'us the data and spec file',Mt1,Mt2,F)
       CALL writln('          that produced this message '//
     &               '(x12@census.gov).',Mt1,Mt2,F)
       CALL abend()
c     ------------------------------------------------------------------
      ELSE IF(info.eq.PACFER)THEN
       CALL writln('Problem calculating the theoretical ARMA '//
     &             'ACF</abbr>.',Mt1,Mt2,T)
       CALL abend()
c     ------------------------------------------------------------------
      ELSE IF(info.eq.PVWPER)THEN
       CALL writln('Problem calculating var(w_p|z)<.',
     &             Mt1,Mt2,T)
       CALL abend()
      END IF
c     ------------------------------------------------------------------
      RETURN
      END
