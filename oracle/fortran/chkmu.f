C     Last change:  BCM  14 Oct 2005    4:33 pm
      SUBROUTINE chkmu(Trnsrs,A,Nefobs,Na,Frstry,Kstep,Lprt)
      IMPLICIT NONE
c     ------------------------------------------------------------------
c     This subroutine performs an automatic ARIMA model selection.  The
c     procedure is similar to that of Gomez and Maravall (1998)
c     ------------------------------------------------------------------
      INCLUDE 'stdio.i'
      INCLUDE 'notset.prm'
      INCLUDE 'srslen.prm'
      INCLUDE 'model.prm'
      INCLUDE 'model.cmn'
      INCLUDE 'mdldat.cmn'
      INCLUDE 'arima.cmn'
      INCLUDE 'error.cmn'
      INCLUDE 'prior.prm'
      INCLUDE 'prior.cmn'
c      INCLUDE 'adj.cmn'
c      INCLUDE 'priadj.cmn'
c      INCLUDE 'priusr.cmn'
      INCLUDE 'extend.cmn'
      INCLUDE 'units.cmn'
c     ------------------------------------------------------------------
      LOGICAL T,F
      PARAMETER(T=.true.,F=.false.)
c     ------------------------------------------------------------------
      DOUBLE PRECISION A,Trnsrs,tval,cval
      LOGICAL argok,Lprt,usermu
      INTEGER igrp,begcol,icol,Kstep,Nefobs,Na,Frstry,kmu
      DIMENSION tval(PB),Trnsrs(PLEN),A(*)
c     ------------------------------------------------------------------
      LOGICAL dpeq
      INTEGER strinx
      EXTERNAL dpeq,strinx 
c     ------------------------------------------------------------------
c     If not in model, add constant regressor
c     ------------------------------------------------------------------
      kmu=strinx(F,Grpttl,Grpptr,1,Ngrptl,'Constant')
      usermu=kmu.gt.0
      IF(kmu.eq.0)THEN
       CALL adrgef(DNOTST,'Constant','Constant',PRGTCN,F,F)
       IF(Lfatal)RETURN
       kmu=strinx(F,Grpttl,Grpptr,1,Ngrptl,'Constant')
      END IF
c     ------------------------------------------------------------------
c     Revise regression matrix
c     ------------------------------------------------------------------
      CALL regvar(Trnsrs,Nobspf,Fctdrp,Nfcst,0,Userx,Bgusrx,Nrusrx,
     &            Priadj,Reglom,Nrxy,Begxy,Frstry,T,Elong)
      IF(Lfatal)RETURN
c     ------------------------------------------------------------------
c     estimate model
c     ------------------------------------------------------------------
      argok=T
      CALL rgarma(T,Mxiter,Mxnlit,F,A,Na,Nefobs,argok)
      IF(.not.argok)THEN
       CALL writln('ERROR: A model estimation error has occurred during 
     &testing for a constant',STDERR,Mt2,T)
       CALL writln('       term within the automatic model identificatio
     &n procedure.  The',STDERR,Mt2,F)
       CALL writln('       error message appears below.',STDERR,Mt2,F)
       CALL prterr(nefobs,F)
       IF(Lfatal)RETURN
       CALL abend()
      END IF
      IF(Lfatal)RETURN
c     ------------------------------------------------------------------
c     Generate t-statistics for regressors
c     ------------------------------------------------------------------
      IF(Convrg)THEN
       CALL genrtt(tval)
c     ------------------------------------------------------------------
       IF(Kstep.eq.0)THEN
        cval=1.96D0
       ELSE
c     ------------------------------------------------------------------
C  IN THE SECOND ROUND (Kstep=1), CVAL IS DECREASED
c     ------------------------------------------------------------------
        cval=1.6D0
c       cvalm1=.5D0
c       cvalm2=1.96D0
       END IF
c     ------------------------------------------------------------------
c     check t-test for constant term, if needed
c     ------------------------------------------------------------------
       icol=Grp(kmu)-1
       IF(DABS(tval(icol)).lt.cval)kmu=-1
      ELSE
       IF(Lprt)WRITE(Mt1,1010)
       WRITE(Mt2,1010)
 1010  FORMAT(/,' NOTE: Cannot perform test for constant term:',/,
     &          '       Model estimation does not converge when ',
     &          'constant term added.',//,
     &          '       Constant term will not be included in regARIMA',
     &          ' model',/)
       kmu=-1
      END IF
c     ------------------------------------------------------------------
c     remove constant regressor if not significant
c     ------------------------------------------------------------------
      IF(kmu.lt.0)THEN
       igrp=strinx(T,Grpttl,Grpptr,1,Ngrptl,'Constant')
       begcol=Grp(igrp-1)
       CALL dlrgef(begcol,Nrxy,1)
       IF(Lfatal)RETURN
c     ------------------------------------------------------------------
c     If model has been changed, regenerate regression matrix
c     ------------------------------------------------------------------
       CALL regvar(Trnsrs,Nobspf,Fctdrp,Nfcst,0,Userx,Bgusrx,Nrusrx,
     &             Priadj,Reglom,Nrxy,Begxy,Frstry,T,Elong)
       IF(Lfatal)RETURN
       IF(Lprt.and.usermu)WRITE(Mt1,1020)
 1020  FORMAT('  Constant term removed from model')
      END IF
c     ------------------------------------------------------------------
      RETURN
      END
