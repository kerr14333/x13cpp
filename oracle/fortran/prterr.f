C     Last change:  SRD  19 Nov 99    6:05 am
      SUBROUTINE prterr(Nefobs,Lauto)
      IMPLICIT NONE
c-----------------------------------------------------------------------
c     prterr.f, Release 1, Subroutine Version 1.7, Modified 14 Feb 1995.
c-----------------------------------------------------------------------
      INCLUDE 'stdio.i'
      INCLUDE 'mdltbl.i'
      INCLUDE 'srslen.prm'
      INCLUDE 'model.prm'
      INCLUDE 'model.cmn'
      INCLUDE 'mdldat.cmn'
      INCLUDE 'hiddn.cmn'
      INCLUDE 'error.cmn'
      INCLUDE 'units.cmn'
      INCLUDE 'tbllog.prm'
      INCLUDE 'tbllog.cmn'
c-----------------------------------------------------------------------
      LOGICAL T,F
      PARAMETER(T=.true.,F=.false.)
c     ------------------------------------------------------------------
      CHARACTER str*(PMDLCR)
      LOGICAL ltmper,Lauto
      INTEGER itmp,nchr,Nefobs,nfil,Begxy,Nrxy,Endspn
      DIMENSION Begxy(2),Endspn(2)
c     ------------------------------------------------------------------
      DOUBLE PRECISION dpmpar
      INTEGER nblank
      EXTERNAL nblank,dpmpar
c-----------------------------------------------------------------------
      COMMON /armaxy/ Endspn,Begxy,Nrxy
c-----------------------------------------------------------------------
      nfil=nblank(Cursrs)
c-----------------------------------------------------------------------
c     Unknown error
c-----------------------------------------------------------------------
      IF(Armaer.eq.PUNKER.or.Armaer.lt.0)THEN
       Convrg=F
       Var=0D0
       IF(Issap.eq.2)THEN
        CALL errhdr
        WRITE(STDERR,1230)Cursrs(1:nfil)
        WRITE(Mt1,1030)
        WRITE(Mt2,1030)
       ELSE IF(Irev.eq.4)THEN
        CALL errhdr
        WRITE(STDERR,1230)Cursrs(1:nfil)
        WRITE(Mt1,1040)
        WRITE(Mt2,1040)
       ELSE
        IF(.NOT.Lauto)THEN
         WRITE(STDERR,1230)Cursrs(1:nfil)
         WRITE(Mt1,1050)
        END IF
        WRITE(Mt2,1050)
       END IF
c-----------------------------------------------------------------------
c     Xy is singular
c-----------------------------------------------------------------------
      ELSE IF(Armaer.eq.PSNGER.or.Armaer.eq.PISNER)THEN
       IF(Sngcol.lt.Ncxy)THEN
        CALL getstr(Colttl,Colptr,Ncoltl,Sngcol,str,nchr)
        IF(Lfatal)RETURN
c     -------------------------------------------------------------------
       ELSE
        nchr=4
        str(1:nchr)='data'
       END IF
c     -------------------------------------------------------------------
       IF(Armaer.eq.PISNER)THEN
        IF(.NOT.Lauto)THEN
         WRITE(STDERR,1230)Cursrs(1:nfil)
         WRITE(Mt1,1060)str(1:nchr)
        END IF
        CALL errhdr
        WRITE(Mt2,1060)str(1:nchr)
c     ------------------------------------------------------------------
       ELSE
        IF(.not.Lauto)THEN
         WRITE(STDERR,1230)Cursrs(1:nfil)
         WRITE(Mt1,1070)str(1:nchr)
c-----------------------------------------------------------------------
c     Added printing of regression matrix (BCM 5-97)
c-----------------------------------------------------------------------
         IF(Prttab(LREGDT))THEN
          CALL prtshd('Regression Matrix',Begxy,Sp,Nrxy,F)
          IF(.not.Lfatal)CALL prtmtx(Begxy,Sp,Xy,Nrxy,Ncxy,Colttl,
     &                               Colptr,Ncoltl)
          IF(Lfatal)RETURN
         END IF
        END IF
        CALL errhdr
        WRITE(Mt2,1070)str(1:nchr)
       END IF
c     -------------------------------------------------------------------
       IF(.not.Lauto)CALL abend()
       RETURN
c-----------------------------------------------------------------------
c     Improper input to the nonlinear routine
c-----------------------------------------------------------------------
      ELSE IF(Armaer.eq.PINPER)THEN
       CALL errhdr
       WRITE(STDERR,1230)Cursrs(1:nfil)
       WRITE(Mt1,1080)
       WRITE(Mt2,1080)
c     ------------------------------------------------------------------
      ELSE IF(Armaer.eq.PMXIER)THEN
       CALL errhdr
       CALL itrerr('iterations',Lauto,Issap,Irev)
c     ------------------------------------------------------------------
      ELSE IF(Armaer.eq.PMXFER)THEN
       CALL errhdr
       CALL itrerr('function evaluations',Lauto,Issap,Irev)
c     ------------------------------------------------------------------
      ELSE IF(Armaer.eq.PSCTER.or.Armaer.eq.PSPMER.or.Armaer.eq.PCOSER)
     &        THEN
       IF(.not.Lauto)THEN
        WRITE(STDERR,1230)Cursrs(1:nfil)
        WRITE(Mt1,1090)
       END IF
       CALL errhdr
       WRITE(Mt2,1090)
       IF(.not.Lprier)THEN
        IF(.not.Lauto)WRITE(Mt1,1100)
        WRITE(Mt2,1100)
       ELSE IF(Armaer.eq.PSCTER)THEN
        IF(.not.Lauto)THEN
         WRITE(STDERR,1230)Cursrs(1:nfil)
         WRITE(Mt1,1110)
        END IF
        WRITE(Mt2,1110)
c     ------------------------------------------------------------------
       ELSE IF(Armaer.eq.PSPMER)THEN
        IF(.not.Lauto)THEN
         WRITE(STDERR,1230)Cursrs(1:nfil)
         WRITE(Mt1,1120)
        END IF
        CALL errhdr
        WRITE(Mt2,1120)
c     ------------------------------------------------------------------
       ELSE IF(Armaer.eq.PCOSER)THEN
        CALL errhdr
        IF(.not.Lauto)THEN
         WRITE(STDERR,1230)Cursrs(1:nfil)
         WRITE(Mt1,1130)
        END IF
        WRITE(Mt2,1130)
       END IF
       IF(Issap.eq.2)THEN
        WRITE(Mt1,1200)
       ELSE IF(Irev.eq.4)THEN
        WRITE(Mt1,1210)
       END IF
c-----------------------------------------------------------------------
c     Invertibility errors.  Print the estimates, and stop.
c-----------------------------------------------------------------------
      ELSE IF(Armaer.eq.PNIFER)THEN
       CALL getstr(Oprttl,Oprptr,Noprtl,Prbfac,str,nchr)
       IF(Lfatal)RETURN
       IF(.not.Lauto)THEN
        WRITE(STDERR,1230)Cursrs(1:nfil)
        WRITE(Mt1,1140)str(1:nchr)
       END IF
       CALL errhdr
       WRITE(Mt2,1140)str(1:nchr)
       IF(Issap.eq.2)THEN
        WRITE(Mt1,1150)
       ELSE IF(Irev.eq.4)THEN
        WRITE(Mt1,1160)
       END IF
       ltmper=Lprier
       Lprier=T
       CALL chkrt2(F,itmp,Lhiddn)
       IF(Lfatal)RETURN
       Lprier=ltmper
       IF(.not.Lauto)CALL abend()
       RETURN
c     ------------------------------------------------------------------
      ELSE IF(Armaer.eq.PNIMER)THEN
       CALL getstr(Oprttl,Oprptr,Noprtl,Prbfac,str,nchr)
       IF(Lfatal)RETURN
       IF(.not.Lauto)THEN
        WRITE(STDERR,1230)Cursrs(1:nfil)
        WRITE(Mt1,1170)str(1:nchr)
       END IF
       CALL errhdr
       WRITE(Mt2,1170)str(1:nchr)
       IF(Issap.eq.2)THEN
        WRITE(Mt1,1150)
       ELSE IF(Irev.eq.4)THEN
        WRITE(Mt1,1160)
       END IF
       ltmper=Lprier
       Lprier=T
       CALL chkrt2(F,itmp,Lhiddn)
       IF(Lfatal)RETURN
       Lprier=ltmper
       IF(.not.Lauto)CALL abend()
       RETURN
c-----------------------------------------------------------------------
c     Stpitr convergence errors
c-----------------------------------------------------------------------
      ELSE IF(Armaer.eq.PCNTER)THEN
       CALL errhdr
       IF(.not.Lauto)THEN
        WRITE(STDERR,1230)Cursrs(1:nfil)
        WRITE(Mt1,1180)2D0/Nefobs*dpmpar(1)
       END IF
       WRITE(Mt2,1180)2D0/Nefobs*dpmpar(1)
       IF(Issap.eq.2)THEN
        WRITE(Mt1,1150)
       ELSE IF(Irev.eq.4)THEN
        WRITE(Mt1,1160)
       END IF
       IF(.not.Lauto)CALL abend()
       RETURN
c     ------------------------------------------------------------------
      ELSE IF(Armaer.eq.PDVTER)THEN
       CALL errhdr
       IF(.not.Lauto)THEN
        WRITE(STDERR,1230)Cursrs(1:nfil)
        WRITE(Mt1,1190)
       END IF
       WRITE(Mt2,1190)
       IF(Issap.eq.2)THEN
        WRITE(Mt1,1200)
       ELSE IF(Irev.eq.4)THEN
        WRITE(Mt1,1210)
       END IF
c-----------------------------------------------------------------------
c     Singular ARMA covariance matrix
c-----------------------------------------------------------------------
      ELSE IF(Armaer.eq.PACSER)THEN
       CALL errhdr
       IF(.not.Lauto)THEN
        WRITE(STDERR,1230)Cursrs(1:nfil)
        WRITE(Mt1,1220)
       END IF
       WRITE(Mt2,1220)
       IF(Issap.eq.2)THEN
        WRITE(Mt1,1200)
       ELSE IF(Irev.eq.4)THEN
        WRITE(Mt1,1210)
       END IF
c     ------------------------------------------------------------------
c     Objective function equal to zero
c     ------------------------------------------------------------------
      ELSE IF(Armaer.eq.POBFN0)THEN
       CALL errhdr
       IF(.not.Lauto)THEN
        WRITE(STDERR,1230)Cursrs(1:nfil)
        WRITE(Mt1,1240)
       END IF
       WRITE(Mt2,1240)
       IF(Issap.eq.2)THEN
        WRITE(Mt1,1200)
       ELSE IF(Irev.eq.4)THEN
        WRITE(Mt1,1210)
       END IF
       IF(.not.Lauto)CALL abend()
      END IF
c     ------------------------------------------------------------------
      RETURN
 1030 FORMAT(/,' ERROR: Nonlinear estimation error with unknown cause ',
     &         'during ',/,'        sliding spans analysis.')
 1040 FORMAT(/,' ERROR: Nonlinear estimation error with unknown cause ',
     &         'during ',/,'        revisions analysis.')
 1050 FORMAT(/,' ERROR: Nonlinear estimation error with unknown cause.',
     &       /)
 1060 FORMAT(/,' ERROR: Regression matrix singular because of ',a,'.',
     &       /,'        Remove variable(s) from regression spec and ',
     &         'try again.',/)
 1070 FORMAT(/,' ERROR: Regression matrix singular because of ',a,'.',
     &       /,'        Check regression model or change automatic ',
     &         'outlier options',
     &       /,'        i.e. method to addone or types to identify AO ',
     &         'only.',/)
 1080 FORMAT(/,' WARNING: Improper input parameters to the likelihood',
     &         'minimization routine.',
     &       /,'          Please send us the data and spec file that ',
     &         'produced this',
     &       /,'          message (x12@census.gov).')
 1090 FORMAT(/,' WARNING: Estimation was terminated because no ',
     &         'further improvement in',
     &       /,'          the likelihood was possible.  Check ',
     &         'iteration output to ',
     &       /,'          confirm that model estimation really ',
     &         'converged.')
 1100 FORMAT(/)
 1110 FORMAT('          Convergence tolerance on the likelihood is ',
     &       'too strict.',/)
 1120 FORMAT(/,' WARNING: Convergence tolerance for the relative ',
     &         'difference in the',
     &       /,'          parameter estimates is too strict.')
 1130 FORMAT(/,'          Cosine of the angle between the vector of ',
     &         'expected values and ',
     &       /,'          any column of the jacobian is too small.',/)
 1140 FORMAT(/,' ERROR: ',a,' has roots inside the unit circle but ',/,
     &         'some',/,
     &         '         parameters are fixed so cannot invert the ',
     &         'operator.',/)
 1150 FORMAT('         This error occurred during the sliding spans ',
     &       'analysis.',/)
 1160 FORMAT('         This error occurred during the history ',
     &       'analysis.',/)
 1170 FORMAT(/,' ERROR: ',a,' has roots inside the unit circle but ',
     &         'some are missing',
     &       /,'        so cannot invert the operator.  Try ',
     &         'including all lags.',/)
 1180 FORMAT(/,' ERROR: Convergence tolerance must be set larger than ',
     &         'machine',
     &       /,'precision',e25.14,'.',/)
 1190 FORMAT(/,' WARNING: Deviance was less than machine precision ',
     &         'so could not',
     &       /,'          calculate the relative deviance.',/)
 1200 FORMAT('          This warning occurred during the sliding spans',
     &       'analysis.',/)
 1210 FORMAT('          This warning occurred during the history ',
     &       'analysis.',/)
 1220 FORMAT(/,' WARNING: The covariance matrix of the ARMA ',
     &         'parameters is singular,',
     &       /,'          so the standard errors and the correlation ',
     &         'matrix of the ARMA',
     &       /,'          parameters will not be printed out.',/)
 1230 FORMAT(/,' Error(s) found while estimating the regARIMA model.',/,
     &       ' For more details, check the error file (',a,'.err).',/)
 1240 FORMAT(/,' ERROR: Differencing has annihilated the series.',/,
     &       '        Check the model specified in the arima spec,',
     &       ' set or change',/,
     &       '        the possible differencing orders (if using the ',
     &       'automdl spec), or',/,
     &       '        change the models specified in the automatic ',
     &       'model file',/,
     &       '        (if using the pickmdl spec).')
      END
