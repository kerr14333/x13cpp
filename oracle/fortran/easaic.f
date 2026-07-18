C     Last change:  BCM  19 Feb 1999   10:39 am
      SUBROUTINE easaic(Trnsrs,A,Nefobs,Na,Frstry,Lester,Lprtit,Lprt,
     &                  Lprtfm,Lsavlg,Lsumm,Lhiddn)
      IMPLICIT NONE
c-----------------------------------------------------------------------
c     Estimate a number of regARIMA model, each with either no easter
c     effect or an easter effect with length 1, 8, or 15.  This routine
c     chooses the model with the lowest value of AICC and prints out the
c     resulting model.
c-----------------------------------------------------------------------
      LOGICAL F,T
      DOUBLE PRECISION ZERO,ONE
      PARAMETER(F=.false.,T=.true.,ZERO=0D0,ONE=1D0)
c-----------------------------------------------------------------------
      INCLUDE 'notset.prm'
      INCLUDE 'srslen.prm'
      INCLUDE 'model.prm'
      INCLUDE 'model.cmn'
      INCLUDE 'mdldat.cmn'
      INCLUDE 'arima.cmn'
      INCLUDE 'lkhd.cmn'
      INCLUDE 'extend.cmn'
      INCLUDE 'units.cmn'
      INCLUDE 'adj.cmn'
      INCLUDE 'x11adj.cmn'
      INCLUDE 'prior.prm'
      INCLUDE 'prior.cmn'
*      INCLUDE 'ssprep.cmn'
      INCLUDE 'error.cmn'
c-----------------------------------------------------------------------
      INTEGER PA
      PARAMETER(PA=PLEN+2*PORDER)
c-----------------------------------------------------------------------
      CHARACTER eastr*(155),temp*(30),fmtsvl*(25)
      LOGICAL Lprt,Lprtit,Lester,argok,lhide,Lprtfm,Lsavlg,Lhiddn,lmanyE
      DOUBLE PRECISION A,aicbst,Trnsrs,aicno,aiceas,thiscv
      INTEGER Frstry,i,Na,Nefobs,begcol,ncol,easgrp,Lsumm,neachr,endlag,
     &        ilag,ieas,ntmp,j,nbnoe,nbe,aicdf
      DIMENSION A(PA),Trnsrs(PLEN)
c-----------------------------------------------------------------------
      INTEGER strinx
      LOGICAL dpeq
      EXTERNAL strinx,dpeq
c-----------------------------------------------------------------------
c     Initialize variables
c-----------------------------------------------------------------------
      IF(.not.Lprt)THEN
       lhide=Lhiddn
       Lhiddn=T
      END IF
      aiceas=DNOTST
      lmanyE=F
c-----------------------------------------------------------------------
c     Set up format for saving AICC results to log file
c-----------------------------------------------------------------------
      IF(Lsavlg)THEN
       CALL mkealb(eastr,neachr,Eastst,Easidx,Easvec(Neasvc)+Easidx,F)
       CALL setchr(' ',25,fmtsvl)
       IF (Neas.gt.0) THEN
        WRITE(fmtsvl,1010)MAX(neachr*Neas,4)+10+Neas-1
       ELSE
        WRITE(fmtsvl,1010)MAX(neachr,4)+10
       END IF
      END IF
c-----------------------------------------------------------------------
c     Start loop through model choices
c-----------------------------------------------------------------------
      IF(Lsumm.gt.0)THEN
       IF(Easvec(Neasvc).eq.99)THEN
        WRITE(Nform,900)'testalleaster','yes'
       ELSE
        WRITE(Nform,900)'testalleaster','no'
       END IF
       WRITE(Nform,1020)'aictest.easter.num',Neasvc-1
      END IF
      DO i=1,Neasvc
c-----------------------------------------------------------------------
c     See if there is an easter effect in the model
c-----------------------------------------------------------------------
       IF(Neas.gt.0)THEN
        DO j=1,Neas 
         easgrp=strinx(T,Grpttl,Grpptr,1,Ngrptl,'Easter')
         IF(easgrp.eq.0)
     &    easgrp=strinx(T,Grpttl,Grpptr,1,Ngrptl,'StatCanEaster')
         IF(easgrp.eq.0)
     &    easgrp=strinx(T,Grpttl,Grpptr,1,Ngrptl,'StockEaster')
c-----------------------------------------------------------------------
c     If easter regressor in model, delete regressor from model
c-----------------------------------------------------------------------
         IF(easgrp.gt.0)THEN
          begcol=Grp(easgrp-1)
          ncol=Grp(easgrp)-begcol
          CALL dlrgef(begcol,Nrxy,ncol)
          IF(Lfatal)RETURN
         END IF
        END DO
        Neas=0
       END IF
c-----------------------------------------------------------------------
c     If i > 1, add new easter regressor to model
c-----------------------------------------------------------------------
       IF(i.gt.1.or.easgrp.gt.0)THEN
        lmanyE=i.eq.Neasvc.and.Easvec(Neasvc).eq.99
        IF(lmanyE)THEN
         ieas=1
         DO j=2,Neasvc-1
          CALL mkealb(temp,ntmp,Eastst,Easidx,Easvec(j)+Easidx,F)
          IF(.not.Lfatal)THEN
           CALL addeas(Easvec(j)+Easidx,Easidx,Eastst)
           eastr(ieas:(ieas+ntmp))=temp(1:ntmp)//'+'
           ieas=ieas+ntmp+1
          END IF
          IF(Lfatal)RETURN
         END DO
         eastr(ieas-1:ieas-1)=' '
         neachr=ieas-2
         Neas=Neasvc-2
        ELSE IF(i.gt.1)THEN
         CALL mkealb(eastr,neachr,Eastst,Easidx,Easvec(i)+Easidx,F)
         IF(.not.Lfatal)THEN
          CALL addeas(Easvec(i)+Easidx,Easidx,Eastst)
          Neas=1
         END IF
         IF(Lfatal)RETURN
        END IF
        IF(.not.Lfatal)
     &     CALL regvar(Trnsrs,Nobspf,Fctdrp,Nfcst,0,Userx,Bgusrx,
     &                 Nrusrx,Priadj,Reglom,Nrxy,Begxy,Frstry,T,Elong)
        IF(Lfatal)RETURN
       END IF
c-----------------------------------------------------------------------
c    If there are ARMA parameters that were set as initial values by
c    the user, reset Arimap to those values (BCM, 9-2010)
c-----------------------------------------------------------------------
       IF(Nopr.gt.0)THEN
        endlag=Opr(Nopr)-1
        DO ilag=1,endlag
         IF(.not.Arimaf(ilag))Arimap(ilag)=Ap1(ilag)
        END DO
       END IF
c-----------------------------------------------------------------------
c     Estimate model
c-----------------------------------------------------------------------
       argok=Lautom.or.Lautox
       CALL rgarma(T,Mxiter,Mxnlit,F,A,Na,Nefobs,argok)
       IF((.not.Lfatal).and.(Lautom.or.Lautox).and.(.not.argok))
     &    CALL abend()
       IF(Lfatal)RETURN
c-----------------------------------------------------------------------
c     If an estimation error is found, discontinue the routine.
c-----------------------------------------------------------------------
       IF(Armaer.eq.PMXIER.or.Armaer.eq.PSNGER.or.Armaer.eq.PISNER.or.
     &    Armaer.eq.PNIFER.or.Armaer.eq.PNIMER.or.Armaer.eq.PCNTER.or.
     &    Armaer.eq.POBFN0.or.Armaer.lt.0.or.
     &    ((Lautom.or.Lautox).and.(.not.argok)))THEN
        Lester=T
        RETURN
c-----------------------------------------------------------------------
c     If only a warning message would be printed out, reset the error
c     indicator variable to zero.
c-----------------------------------------------------------------------
       ELSE IF(Armaer.ne.0)THEN
        Armaer=0
       END IF
c-----------------------------------------------------------------------
c     Compute the likelihood statistics and AICC for the model
c-----------------------------------------------------------------------
       IF(i.eq.1)THEN
        IF(Lprt)WRITE(Mt1,1030)
       ELSE
        IF(Lprt)WRITE(Mt1,1040)eastr(1:neachr)
       END IF
       IF(i.eq.Neasvc)THEN
        CALL prlkhd(Y(Frstsy),Adj(Adj1st),Adjmod,Fcntyp,Lam,F,Lprt,
     &              Lprtfm)
       ELSE
        CALL prlkhd(Y(Frstsy),Adj(Adj1st),Adjmod,Fcntyp,Lam,F,Lprt,F)
       END IF
       IF(Lfatal)RETURN
c-----------------------------------------------------------------------
c     See if this AICC is the smallest.  If so, update value and index
c     of best AICC.
c-----------------------------------------------------------------------
       IF(i.eq.1)THEN
        aicno=Aicc
        nbnoe=Nb
        IF(Lsavlg)WRITE(Ng,fmtsvl)'AICC(no easter)',':',Aicc
        IF(Lsumm.gt.0)WRITE(Nform,1050)'noeaster',Aicc
       ELSE
        IF(Lsavlg)WRITE(Ng,fmtsvl)'AICC('//eastr(1:neachr)//')',':',Aicc
        IF(Lsumm.gt.0)THEN
         IF(lmanyE)THEN
          WRITE(Nform,1050)'alleaster',Aicc
         ELSE
          WRITE(Nform,1060)'easter',Easvec(i),Aicc
         END IF
        END IF
        IF(i.eq.2)THEN
         aiceas=Aicc
         Aicind=Easvec(i)
         nbe=Nb
        ELSE
         IF(lmanyE)THEN
          IF(.not.dpeq(Pvaic,DNOTST))THEN
           aicdf=Nb-nbe
           CALL chsppf(Pvaic,aicdf,thiscv,Mt1)
           Rgaicd(PEAIC)=thiscv-2D0*DBLE(aicdf)
          END IF
         END IF
        END IF
        Dfaice=aiceas-Aicc
        IF(Dfaice.gt.Rgaicd(PEAIC))THEN
         Aicind=Easvec(i)
         aiceas=Aicc
         IF(.not.dpeq(Pvaic,DNOTST))nbe=Nb
        END IF
       END IF
      END DO
c-----------------------------------------------------------------------
      Dfaice=aicno-aiceas
      IF(.not.dpeq(Pvaic,DNOTST))THEN
       aicdf=nbe-nbnoe
       CALL chsppf(Pvaic,aicdf,thiscv,Mt1)
       Rgaicd(PEAIC)=thiscv-2D0
      END IF
      IF(Dfaice.gt.Rgaicd(PEAIC))THEN
       aicbst=aiceas
      ELSE
       aicbst=Aicno
       Aicind=-1
      END IF
c-----------------------------------------------------------------------
      IF(.not.Lprt)Lhiddn=lhide
c-----------------------------------------------------------------------
c     Show Easter effect that aic prefers
c-----------------------------------------------------------------------
      IF(Lprt)THEN
       IF(Aicind.lt.0)THEN
        IF(dpeq(Pvaic,DNOTST))THEN
         WRITE(Mt1,1070)Rgaicd(PEAIC)
        ELSE
         WRITE(Mt1,1100)ONE-Pvaic,Rgaicd(PEAIC)
        END IF
        IF(Finhol)Finhol=F
       ELSE
        IF(Easidx.eq.0)THEN
         IF(Eastst.eq.1)THEN
          IF(dpeq(Pvaic,DNOTST))THEN
           IF(Aicind.eq.99)THEN
            IF(neachr.le.32)THEN
             WRITE(Mt1,1080)Rgaicd(PEAIC),eastr(1:neachr)
            ELSE IF(neachr.le.54)THEN
             WRITE(Mt1,1081)Rgaicd(PEAIC),eastr(1:neachr)
            ELSE
             WRITE(Mt1,1082)Rgaicd(PEAIC),eastr(1:neachr)
            END IF
           ELSE
            WRITE(Mt1,1090)Rgaicd(PEAIC),'Easter',Aicind
           END IF
          ELSE
           IF(Aicind.eq.99)THEN
            IF(neachr.le.32)THEN
             WRITE(Mt1,1110)ONE-Pvaic,Rgaicd(PEAIC),eastr(1:neachr)
            ELSE IF(neachr.le.54)THEN
             WRITE(Mt1,1111)ONE-Pvaic,Rgaicd(PEAIC),eastr(1:neachr)
            ELSE
             WRITE(Mt1,1112)ONE-Pvaic,Rgaicd(PEAIC),eastr(1:neachr)
            END IF
           ELSE
            WRITE(Mt1,1120)ONE-Pvaic,Rgaicd(PEAIC),'Easter',Aicind
           END IF
          END IF
         ELSE
          IF(dpeq(Pvaic,DNOTST))THEN
           WRITE(Mt1,1090)Rgaicd(PEAIC),'Stock Easter',Aicind
          ELSE
           WRITE(Mt1,1120)ONE-Pvaic,Rgaicd(PEAIC),'Stock Easter',Aicind
          END IF
         END IF
        ELSE
         IF(dpeq(Pvaic,DNOTST))THEN
          WRITE(Mt1,1090)
     &          Rgaicd(PEAIC),'Statistics Canada Easter',Aicind
         ELSE
          WRITE(Mt1,1120)
     &         ONE-Pvaic,Rgaicd(PEAIC),'Statistics Canada Easter',Aicind
         END IF
        END IF
       END IF
      END IF
c-----------------------------------------------------------------------
c     If model with best AICC wasn't the last one estimated, redo model
c     estimation so the best model is returned.
c-----------------------------------------------------------------------
      IF(aicind.lt.Easvec(Neasvc))THEN
       DO j=1,Neas
        easgrp=strinx(T,Grpttl,Grpptr,1,Ngrptl,'Easter')
        IF(easgrp.eq.0)
     &    easgrp=strinx(T,Grpttl,Grpptr,1,Ngrptl,'StatCanEaster')
        IF(easgrp.eq.0)
     &    easgrp=strinx(T,Grpttl,Grpptr,1,Ngrptl,'StockEaster')
        begcol=Grp(easgrp-1)
        ncol=Grp(easgrp)-begcol
        CALL dlrgef(begcol,Nrxy,ncol)
       END DO
c-----------------------------------------------------------------------
c     Add new Easter variable, if necessary
c-----------------------------------------------------------------------
       IF(.not.Lfatal.and.aicind.ge.0)
     &    CALL addeas(aicind+Easidx,Easidx,Eastst)
c-----------------------------------------------------------------------
c    If there are ARMA parameters that were set as initial values by
c    the user, reset Arimap to those values (BCM, 9-2010)
c-----------------------------------------------------------------------
       IF(Nopr.gt.0)THEN
        endlag=Opr(Nopr)-1
        DO ilag=1,endlag
         IF(.not.Arimaf(ilag))Arimap(ilag)=Ap1(ilag)
        END DO
       END IF
c-----------------------------------------------------------------------
c     Estimate model
c-----------------------------------------------------------------------
       IF(.not.Lfatal)
     &    CALL regvar(Trnsrs,Nobspf,Fctdrp,Nfcst,0,Userx,Bgusrx,Nrusrx,
     &                Priadj,Reglom,Nrxy,Begxy,Frstry,T,Elong)
       IF(.not.Lfatal)
     &    CALL rgarma(T,Mxiter,Mxnlit,Lprtit,A,Na,Nefobs,argok)
       IF((.not.Lfatal).and.(Lautom.or.Lautox).and.(.not.argok))
     &    Lester=T
      END IF
c-----------------------------------------------------------------------
  900 FORMAT(a,': ',a)
 1010 FORMAT('(1x,a,t',i3.3,',a,1x,f15.4)')
 1020 FORMAT(a,':',i5)
 1030 FORMAT(//,' Likelihood statistics for model without Easter')
 1040 FORMAT(//,' Likelihood statistics for model with ',a)
 1050 FORMAT('aictest.e.aicc.',a,': ',e29.15)
 1060 FORMAT('aictest.e.aicc.',a,i2.2,': ',e29.15)
 1070 FORMAT(//,'   *****   AICC (with aicdiff=',F7.4,
     &          ') prefers model without Easter   *****')
 1080 FORMAT(//,'   *****   AICC (with aicdiff=',F7.4,
     &          ') prefers model with ',a,t67,'*****')
 1081 FORMAT(//,'   *****   AICC (with aicdiff=',F7.4,
     &          ') prefers model with ',/,'   *****  ',a,t67,'*****')
 1082 FORMAT(//,'   *****   AICC (with aicdiff=',F7.4,
     &          ') prefers model with ',/,'   *****  ',a,'   *****')
 1090 FORMAT(//,'   *****   AICC (with aicdiff=',F7.4,
     &          ') prefers model with ',a,'[',i2,']   *****')
 1100 FORMAT(//,'   *****   AICC (with p-value = ',F7.5,' and aicdiff=',
     &          F7.4,') prefers model without Easter   *****')
 1110 FORMAT(//,'   *****   AICC (with p-value = ',F7.5,' and aicdiff=',
     &          F7.4,') prefers model with ',a,'   *****')
 1111 FORMAT(//,'   *****   AICC (with p-value = ',F7.5,' and aicdiff=',
     &          F7.4,') prefers model with ',/,'   *****  ',a,t67,
     &          '*****')
 1112 FORMAT(//,'   *****   AICC (with p-value = ',F7.5,' and aicdiff=',
     &          F7.4,') prefers model with ',/,'   *****  ',a,
     &          '   *****')
 1120 FORMAT(//,'   *****   AICC (with p-value = ',F7.5,' and aicdiff=',
     &          F7.4,') prefers model with ',a,'[',i2,']   *****')
c-----------------------------------------------------------------------
      RETURN
      END
