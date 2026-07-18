C     Last change:  BCM  23 Mar 2005    9:23 am
      SUBROUTINE lomaic(Trnsrs,A,Nefobs,Na,Frstry,Lester,Lprtit,Lprt,
     &                  Lprtfm,Lsavlg,Lsumm,Lhiddn)
      IMPLICIT NONE
c-----------------------------------------------------------------------
c     Estimate two regARIMA models, one with user-defined regressors and
c     one without.  This routine chooses the model with the lowest value
c     of AICC and prints out the resulting model.
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
      INCLUDE 'picktd.cmn'
      INCLUDE 'arima.cmn'
      INCLUDE 'lkhd.cmn'
      INCLUDE 'extend.cmn'
      INCLUDE 'units.cmn'
      INCLUDE 'adj.cmn'
      INCLUDE 'prior.prm'
      INCLUDE 'prior.cmn'
      INCLUDE 'error.cmn'
c-----------------------------------------------------------------------
      INTEGER PA
      PARAMETER(PA=PLEN+2*PORDER)
c-----------------------------------------------------------------------
      CHARACTER lnstr*(30),effttl*(PCOLCR),creg2*(6)
      LOGICAL Lprt,Lprtit,Lester,argok,lhide,Lprtfm,Lsavlg,Lhiddn,lreest
      DOUBLE PRECISION A,aicnol,aiclom,Trnsrs,thiscv
      INTEGER Frstry,i,Na,Nefobs,nchr,klm,nlnchr,ncreg2,ilom,Lsumm
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
      CALL mklnlb(lnstr,nlnchr,creg2,ncreg2,Lomtst,Lndate,Lnzero,Sp)
      IF(Lomtst.eq.1)THEN
       klm=strinx(T,Grpttl,Grpptr,1,Ngrptl,'Length-of-Month')
      ELSE IF(Lomtst.eq.2)THEN
       klm=strinx(T,Grpttl,Grpptr,1,Ngrptl,'Length-of-Quarter')
      ELSE IF(Lomtst.eq.3)THEN
       klm=strinx(T,Grpttl,Grpptr,1,Ngrptl,'Leap Year')
      END IF
      lreest=F
c-----------------------------------------------------------------------
c     Estimate model with lom/loq/lpyear regressors
c-----------------------------------------------------------------------
      CALL regvar(Trnsrs,Nobspf,Fctdrp,Nfcst,0,Userx,Bgusrx,Nrusrx,
     &            Priadj,Reglom,Nrxy,Begxy,Frstry,T,Elong)
      IF(Lfatal)RETURN
      argok=Lautom.or.Lautox
      CALL rgarma(T,Mxiter,Mxnlit,F,A,Na,Nefobs,argok)
      IF((.not.Lfatal).and.(Lautom.or.Lautox).and.(.not.argok))
     &   CALL abend()
      IF(Lfatal)RETURN
c-----------------------------------------------------------------------
c     If an estimation error is found, discontinue the routine.
c-----------------------------------------------------------------------
      IF(Armaer.eq.PMXIER.or.Armaer.eq.PSNGER.or.Armaer.eq.PISNER.or.
     &   Armaer.eq.PNIFER.or.Armaer.eq.PNIMER.or.Armaer.eq.PCNTER.or.
     &   Armaer.eq.POBFN0.or.Armaer.lt.0.or.
     &   ((Lautom.or.Lautox).and..not.argok))THEN
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
      IF(Lprt)THEN
       IF(klm.gt.0)THEN
        WRITE(Mt1,1010)lnstr(1:nlnchr)
       ELSE
        WRITE(Mt1,1020)lnstr(1:nlnchr)
       END IF
      END IF
      CALL prlkhd(Y(Frstsy),Adj(Adj1st),Adjmod,Fcntyp,Lam,F,Lprt,F)
      IF(Lfatal)RETURN
      IF(klm.gt.0)THEN
       aiclom=Aicc
       IF(Lsavlg)WRITE(Ng,1011)lnstr(1:nlnchr),Aicc
       IF(Lsumm.gt.0)
     &    WRITE(Nform,1012)creg2(1:ncreg2),creg2(1:ncreg2),Aicc
      ELSE 
       aicnol=Aicc
       IF(Lsavlg)WRITE(Ng,1021)creg2(1:ncreg2),Aicc
       IF(Lsumm.gt.0)
     &    WRITE(Nform,1012)creg2(1:ncreg2),'no'//creg2(1:ncreg2),Aicc
      END IF
c-----------------------------------------------------------------------
c     If lom/loq/lpyear regressor is not in model, add it to model
c-----------------------------------------------------------------------
      IF(klm.eq.0)THEN
       CALL addlom(Lndate,Lnzero,Sp,Lomtst)
       IF(Lfatal)RETURN
       IF(Lomtst.eq.1)THEN
        klm=strinx(T,Grpttl,Grpptr,1,Ngrptl,'Length-of-Month')
       ELSE IF(Lomtst.eq.2) THEN
        klm=strinx(T,Grpttl,Grpptr,1,Ngrptl,'Length-of-Quarter')
       ELSE
        klm=strinx(T,Grpttl,Grpptr,1,Ngrptl,'Leap Year')
       END IF
      ELSE
c-----------------------------------------------------------------------
c     remove all lom/loq/lpyear regressors from the regression matrix.
c-----------------------------------------------------------------------
       ilom=1
       DO WHILE (ilom.gt.0)
        ilom=strinx(T,Colttl,Colptr,1,Ncoltl,'Length-of-')
        IF(ilom.eq.0)ilom=strinx(T,Colttl,Colptr,1,Ncoltl,'Leap Year')
        IF(ilom.gt.0)THEN
         CALL dlrgef(ilom,Nrxy,1)
         IF(Lfatal)RETURN
        END IF
       END DO
       klm=0
      END IF
c-----------------------------------------------------------------------
c     Re-estimate the updated model
c-----------------------------------------------------------------------
      CALL regvar(Trnsrs,Nobspf,Fctdrp,Nfcst,0,Userx,Bgusrx,Nrusrx,
     &            Priadj,Reglom,Nrxy,Begxy,Frstry,T,Elong)
      IF(.not.Lfatal)CALL rgarma(T,Mxiter,Mxnlit,F,A,Na,Nefobs,argok)
      IF((.not.Lfatal).and.(Lautom.or.Lautox).and.(.not.argok))
     &   CALL abend()
      IF(Lfatal)RETURN
c-----------------------------------------------------------------------
c     If an estimation error is found, discontinue the routine.
c-----------------------------------------------------------------------
      IF(Armaer.eq.PMXIER.or.Armaer.eq.PSNGER.or.Armaer.eq.PISNER.or.
     &   Armaer.eq.PNIFER.or.Armaer.eq.PNIMER.or.Armaer.eq.PCNTER.or.
     &   Armaer.eq.POBFN0.or.Armaer.lt.0.or.((Lautom.or.Lautox).and.
     &   .not.argok))THEN
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
      IF(Lprt)THEN
       IF(klm.gt.0)THEN
        WRITE(Mt1,1010)lnstr(1:nlnchr)
       ELSE
        WRITE(Mt1,1020)lnstr(1:nlnchr)
       END IF
      END IF
      CALL prlkhd(Y(Frstsy),Adj(Adj1st),Adjmod,Fcntyp,Lam,F,Lprt,Lprtfm)
      IF(klm.gt.0)THEN
       aiclom=Aicc
       IF(Lsavlg)WRITE(Ng,1011)creg2(1:ncreg2),Aicc
       IF(Lsumm.gt.0)
     &    WRITE(Nform,1012)creg2(1:ncreg2),creg2(1:ncreg2),Aicc
      ELSE 
       aicnol=Aicc
       IF(Lsavlg)WRITE(Ng,1021)creg2(1:ncreg2),Aicc
       IF(Lsumm.gt.0)
     &    WRITE(Nform,1012)creg2(1:ncreg2),'no'//creg2(1:ncreg2),Aicc
      END IF
      IF(.not.Lprt)Lhiddn=lhide
c-----------------------------------------------------------------------
c     Show the regression model AICC prefers
c-----------------------------------------------------------------------
      Dfaicl=aicnol-aiclom
      IF(.not.dpeq(Pvaic,DNOTST))THEN
       CALL chsppf(Pvaic,1,thiscv,Mt1)
       Rgaicd(PLAIC)=thiscv-2D0
      END IF
      IF(Dfaicl.gt.Rgaicd(PLAIC))THEN
       IF(Lprt)THEN
        IF(dpeq(Pvaic,DNOTST))THEN
         WRITE(Mt1,1030)Rgaicd(PLAIC),'with',lnstr(1:nlnchr)
        ELSE
         WRITE(Mt1,1040)ONE-Pvaic,Rgaicd(PLAIC),'with',lnstr(1:nlnchr)
        END IF
       END IF
c-----------------------------------------------------------------------
c     If no lom/loq/lpyear regressors, add them back to model
c-----------------------------------------------------------------------
       IF(klm.eq.0)THEN
        CALL restor(T,F,F)
        CALL regvar(Trnsrs,Nobspf,Fctdrp,Nfcst,0,Userx,Bgusrx,Nrusrx,
     &              Priadj,Reglom,Nrxy,Begxy,Frstry,T,Elong)
        IF((.not.Lfatal).and.Iregfx.ge.2)THEN
         CALL rmfix(trnsrs,Nbcst,Nrxy,1)
         IF(.not.Lfatal)
     &    CALL regvar(trnsrs,Nobspf,Fctdrp,Nfcst,0,Userx,Bgusrx,Nrusrx,
     &                Priadj,Reglom,Nrxy,Begxy,frstry,T,Elong)
        END IF
        lreest=T
       END IF
      ELSE
       IF(Lprt)THEN
        IF(dpeq(Pvaic,DNOTST))THEN
         WRITE(Mt1,1030)Rgaicd(PLAIC),'without',lnstr(1:nlnchr)
        ELSE
         WRITE(Mt1,1040)
     &         ONE-Pvaic,Rgaicd(PLAIC),'without',lnstr(1:nlnchr)
        END IF
       END IF
       IF(klm.gt.0)THEN
        ilom=1
        DO WHILE (ilom.gt.0)
         ilom=strinx(T,Colttl,Colptr,1,Ncoltl,'Length-of-')
         IF(ilom.eq.0)ilom=strinx(T,Colttl,Colptr,1,Ncoltl,'Leap Year')
         IF(ilom.gt.0)THEN
          CALL dlrgef(ilom,Nrxy,1)
          IF(Lfatal)RETURN
         END IF
        END DO
        lreest=T
       END IF
      END IF
c-----------------------------------------------------------------------
c     Estimate model
c-----------------------------------------------------------------------
      IF(lreest)THEN
       CALL regvar(Trnsrs,Nobspf,Fctdrp,Nfcst,0,Userx,Bgusrx,Nrusrx,
     &             Priadj,Reglom,Nrxy,Begxy,Frstry,T,Elong)
       IF(.not.Lfatal)CALL rgarma(T,Mxiter,Mxnlit,Lprtit,A,Na,Nefobs,
     &                            argok)
       IF((.not.Lfatal).and.(Lautom.or.Lautox).and.(.not.argok))Lester=T
      END IF
c-----------------------------------------------------------------------
 1010 FORMAT(//,' Likelihood statistics for model with ',a,
     &            ' regressors')
 1011 FORMAT(' AICC(',a,')',t27,': ',f15.4)
 1012 FORMAT('aictest.',a,'.aicc.',a,': ',e29.15)
 1020 FORMAT(//,' Likelihood statistics for model without ',a,
     &            ' regressors')
 1021 FORMAT(' AICC(no ',a,')',t27,': ',f15.4)
 1030 FORMAT(//,'   *****   AICC (with aicdiff=',F7.4,
     &          ') prefers model ',a,1x,a,' regressor   *****')
 1040 FORMAT(//,'   *****   AICC (with p-value=',F7.5,' and aicdiff=',
     &          F7.4,') prefers model ',a,1x,a,' regressor   *****')
      RETURN
      END
