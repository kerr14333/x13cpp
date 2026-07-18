C     Last change:  BCM  13 May 1998    9:04 am
      SUBROUTINE prtamd(Cmodel,Mape,Blchi,Qchi,Dgfchi,Mdlnum,Lfcst,
     &                  Ovrdff,Ovrsdf,Fctok,Argok)
      IMPLICIT NONE
c-----------------------------------------------------------------------
c     Print out model information for automatic model identification
c     procedure
c-----------------------------------------------------------------------
      DOUBLE PRECISION CHILIM
      PARAMETER(CHILIM=0.005D0)
c-----------------------------------------------------------------------
      INCLUDE 'notset.prm'
      INCLUDE 'srslen.prm'
      INCLUDE 'model.prm'
      INCLUDE 'model.cmn'
      INCLUDE 'fxreg.cmn'
      INCLUDE 'units.cmn'
      INCLUDE 'title.cmn'
      INCLUDE 'error.cmn'
c-----------------------------------------------------------------------
      CHARACTER Autofl*(PFILMD),Cmodel*(*)
      DOUBLE PRECISION Fctlim,Bcklim,Qlim,Ovrdif,Mape,Blchi,Qchi
      LOGICAL Lfcst,Lautox,Ovrdff,Ovrsdf,Pck1st,Outfer,Id1st,Fctok,Argok
      INTEGER Mdlnum,Dgfchi
      DIMENSION Mape(4)
c-----------------------------------------------------------------------
      LOGICAL dpeq
      EXTERNAL dpeq
c-----------------------------------------------------------------------
      COMMON /armamx/ Fctlim,Bcklim,Qlim,Ovrdif,Lautox,Pck1st,Id1st,
     &                Outfer,Autofl
c-----------------------------------------------------------------------
      IF(.not.Lcmpaq)WRITE(Mt1,'()')
      WRITE(Mt1,1010)Mdlnum,Cmodel
 1010 FORMAT(/,' Model ',i3,': ',a)
      IF(Ngrp.gt.0)
     &   CALL desreg('Regression Model',Ngrp,Grpttl,Grpptr,Ngrptl)
      IF(Ngrpfx.gt.0)
     &   CALL desreg('Regression Model (fixed)',Ngrpfx,Gfxttl,Gfxptr,
     &               Ngfxtl)
      IF(Lfatal)RETURN
c-----------------------------------------------------------------------
c     Print out message when error occurs in producing backcast error,
c     return from subroutine.  BCM May 2007
c-----------------------------------------------------------------------
      IF(.not.Argok)THEN
       WRITE(Mt1,1072)
 1072  FORMAT('  Estimation error in computing average backcast ',
     &          'error for this model.')
       RETURN
      END IF
c-----------------------------------------------------------------------
c     print out average forecast error
c-----------------------------------------------------------------------
      IF(Fctok)CALL prafce(Mt1,Mape,Outfer,Lfcst)
      IF(Lfcst)THEN
       IF(.not.dpeq(Blchi,DNOTST))THEN
        IF(Blchi.gt.CHILIM)then
         WRITE(Mt1,1040)Blchi,Qchi,Dgfchi
        ELSE
         WRITE(Mt1,1041)Blchi,Qchi,Dgfchi
        END IF
       END IF
 1040  FORMAT('  Chi Square Probability:   ',f6.2,' %  (Q = ',f12.4,
     &        ', ',i4,' DF)',/)
 1041  FORMAT('  Chi Square Probability:   ',e17.10,' %  (Q = ',f12.4,
     &        ', ',i4,' DF)',/)
c-----------------------------------------------------------------------
c     Print out model information.  First, load estimated model
c     parameters into temporary variables, then print out model
c     coefficents for each type.
c-----------------------------------------------------------------------
       CALL setpt(Mt1,AR,'Nonseasonal AR')
       IF(.not.Lfatal)CALL setpt(Mt1,MA,'Nonseasonal MA')
       IF(.not.Lfatal)CALL setpt(Mt1,AR,'Seasonal AR')
       IF(.not.Lfatal)CALL setpt(Mt1,MA,'Seasonal MA')
       IF(Lfatal)RETURN
c-----------------------------------------------------------------------
c     Check to see if model has been rejected, and print out message
c     explaining rejection.
c-----------------------------------------------------------------------
       IF((.not.Fctok).or.(Mape(4).gt.Fctlim).or.((Blchi.lt.Qlim).or.
     &    dpeq(Blchi,DNOTST)).or.Ovrdff)THEN
c     &    Ovrdff.or.Ovrsdf)THEN
        IF(.not.Lcmpaq)WRITE(Mt1,'()')
        WRITE(Mt1,1060)Mdlnum
 1060   FORMAT(/,' MODEL ',i3,' REJECTED: ')
        IF(Fctok)THEN
         IF(Mape(4).gt.Fctlim)WRITE(Mt1,1070)Fctlim
 1070    FORMAT('  Average forecast error > ',f6.2,'%')
        ELSE
         WRITE(Mt1,1071)
 1071    FORMAT('  Insufficient data to compute the average forecast ',
     &          'error for this model.')
        END IF
        IF(dpeq(Blchi,DNOTST))THEN
         WRITE(Mt1,1079)
 1079    FORMAT('  Insufficient data to compute the Ljung-Box chi-',
     &          'square probability for this model.')
        ELSE
         IF(Blchi.le.Qlim)THEN
          IF(Qlim.gt.CHILIM)THEN
           WRITE(Mt1,1080)Qlim
          ELSE
           WRITE(Mt1,1081)Qlim
          END IF
         END IF
 1080    FORMAT('  Ljung-Box Q chi-square probability  < ',f6.2,' %')
 1081    FORMAT('  Ljung-Box Q chi-square probability  < ',e17.10,' %')
        END IF
        IF(Ovrdff)WRITE(Mt1,1090)'E','nonseasonal','.'
 1090   FORMAT('  ',a,'vidence of ',a,' overdifferencing',a)
       END IF
      ELSE IF(Mape(4).gt.Bcklim)THEN
       IF(.not.Lcmpaq)WRITE(Mt1,'()')
       WRITE(Mt1,1100)Mdlnum,Bcklim
 1100  FORMAT(/,' MODEL ',i3,' REJECTED: ',/,
     &        '   Average backcast error > ',f6.2,'%')
      END IF
      IF(Ovrsdf)WRITE(Mt1,1090)'WARNING: E','seasonal',
     &                         ' (see message below).'
c-----------------------------------------------------------------------
      RETURN
      END
