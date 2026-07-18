C     last change: Oct.25, 2023 based on BCM's fixes to construct
C     regression matrix correctly if user defined holiday regressors
C     are removed from the model
C     Last change:  BCM  25 Sep 2008    9:23 am
      SUBROUTINE chkchi(Trnsrs,A,Nefobs,Na,Frstry,Lester,Lprtit,Lprchi,
     &                  Lsvchi,Lsvlch)
      IMPLICIT NONE
c-----------------------------------------------------------------------
c     estimate chi-square for groups of user defined holiday regressors
c     and delete the groups that are not significant.
c-----------------------------------------------------------------------
      INCLUDE 'srslen.prm'
      INCLUDE 'model.prm'
      INCLUDE 'notset.prm'
      INCLUDE 'arima.cmn'
      INCLUDE 'model.cmn'
      INCLUDE 'mdldat.cmn'
      INCLUDE 'usrreg.cmn'
      INCLUDE 'prior.prm'
      INCLUDE 'prior.cmn'
      INCLUDE 'extend.cmn'
      INCLUDE 'error.cmn'
      INCLUDE 'units.cmn'
c-----------------------------------------------------------------------
      LOGICAL F,T
      DOUBLE PRECISION ONEHND,ZERO
      INTEGER PA,PTBLWD
      PARAMETER(F=.false.,T=.true.,ONEHND=100D0,ZERO=0D0,
     &          PA=PLEN+2*PORDER,PTBLWD=PGRPCR+6)
c-----------------------------------------------------------------------
      CHARACTER str*(PGRPCR)
      DOUBLE PRECISION Trnsrs,A,xpxinv,tmp,pv,chi2vl
      INTEGER iuhl,Nefobs,Na,nelt,iuser,idel,igrp,begcol,endcol,icol,
     &        rtype,nfix,regidx,nchr,baselt,info,df,tbwdth,i,ncol,
     &        Frstry,j,jcol
      LOGICAL argok,lprthd,lprund,luhl,Lprchi,Lsvchi,Lsvlch,Lester,
     &        Lprtit
      DIMENSION Trnsrs(PLEN),A(PA),xpxinv(PXPX),tmp(2),regidx(PB),
     &          iuhl(PUHLGP),luhl(PUHLGP)
c-----------------------------------------------------------------------
c     Estimate current regARIMA model
c-----------------------------------------------------------------------
      argok=Lautom.or.Lautox
      CALL rgarma(T,Mxiter,Mxnlit,F,A,Na,Nefobs,argok)
      IF((.not.Lfatal).and.(Lautom.or.Lautox).and.(.not.argok))Lester=T
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
c      initialize xpxinv to zero
c-----------------------------------------------------------------------
      CALL setdp(ZERO,PXPX,xpxinv)
c-----------------------------------------------------------------------
c     Get X'X inverse.
c-----------------------------------------------------------------------
      nelt=Ncxy*(Ncxy+1)/2
      CALL copy(Chlxpx,nelt,1,xpxinv)
      CALL dppdi(xpxinv,Nb,tmp,1)
c-----------------------------------------------------------------------
      lprthd=F
      lprund=F
      IF(Lprchi)lprthd=T
      iuser=0
      tbwdth=PTBLWD
c-----------------------------------------------------------------------
      nfix=0
      DO igrp=1,Ngrp
       begcol=Grp(igrp-1)
       endcol=Grp(igrp)-1
       DO icol=begcol,endcol
        IF(Regfx(icol))THEN
         nfix=nfix+1
         regidx(icol)=NOTSET
        ELSE
         regidx(icol)=icol-nfix
        END IF
       END DO
      END DO
c-----------------------------------------------------------------------
      DO igrp=1,Ngrp
       begcol=Grp(igrp-1)
       rtype=Rgvrtp(begcol)
c-----------------------------------------------------------------------
       IF(rtype.ge.PRGTUH.and.rtype.le.PRGUH5)THEN
        endcol=Grp(igrp)-1
        iuser=iuser+1
        iuhl(iuser)=igrp
        IF(Lprchi)lprund=T
        CALL getstr(Grpttl,Grpptr,Ngrp,igrp,str,nchr)
        IF(Lfatal)RETURN
        info=0
        df=endcol-begcol+1
        baselt=regidx(begcol)
c-----------------------------------------------------------------------
        IF(Iregfx.eq.2)THEN
         IF(baselt.eq.NOTSET)df=df-1
         DO icol=begcol+1,endcol
          IF(regidx(icol).eq.NOTSET)THEN
           df=df-1
          ELSE
           baselt=regidx(icol)
          END IF
         END DO
        END IF
c-----------------------------------------------------------------------
        IF(baselt.ne.NOTSET)
     &     CALL chitst(xpxinv,begcol,endcol,chi2vl,pv,regidx,T,info)
        CALL savchi(Lsvchi,Lsvlch,lprthd,tbwdth,baselt,str,nchr,info,
     &              df,chi2vl,pv,'User-defined Holiday Regressors',
     &              'chitst$')
        IF(Lprchi)THEN
         CALL prtchi(Mt1,lprthd,tbwdth,baselt,str,nchr,info,df,chi2vl,
     &               pv,'User-defined Holiday Regressors')
         IF(lprthd)lprthd=F
        END IF
        luhl(iuser)=pv.lt.Chi2cv
       END IF
      END DO
c-----------------------------------------------------------------------
      IF(Lprchi.or.Lsvlch)lprthd=T
      idel=0
      DO i=1,iuser
c-----------------------------------------------------------------------
c    Check to see if the chi square statistic is insignificant
c-----------------------------------------------------------------------
       IF(.not.luhl(i))THEN
c-----------------------------------------------------------------------
c    If so, print output to output or log file
c-----------------------------------------------------------------------
        IF(Lprchi.or.Lsvlch)THEN
         IF(lprthd)THEN
          IF(Lprchi)WRITE(Mt1,1010)Chi2cv*ONEHND
          IF(Lsvlch)WRITE(Ng,1010)Chi2cv*ONEHND
          lprthd=F
         END IF
         CALL getstr(Grpttl,Grpptr,Ngrp,iuhl(i),str,nchr)
         IF(Lfatal)RETURN
         IF(Lprchi)WRITE(Mt1,1020)str(1:nchr)
         IF(Lsvlch)WRITE(Ng,1020)str(1:nchr)
        END IF
       END IF
c-----------------------------------------------------------------------
c    Count number of regression groups deleted
c-----------------------------------------------------------------------
       idel=idel+1
      END DO
c-----------------------------------------------------------------------
c    If any groups are insignificant, delete them, starting from the
c    back and working forward
c-----------------------------------------------------------------------
      IF(idel.gt.0)THEN
       icol = Grp(iuhl(1)-1)
       DO i=iuser,1,-1
        IF(.not.luhl(i))THEN
         igrp=iuhl(i)
         begcol=Grp(igrp-1)
         ncol=Grp(igrp)-begcol
         CALL dlrgef(begcol,Nrxy,ncol)
         IF(Lfatal)RETURN
c-----------------------------------------------------------------------
c     Update User-defined regressors - changed by BCM June 2023
c-----------------------------------------------------------------------
         DO j=ncol,1,-1
          jcol=begcol-icol+1
          CALL dlusrg(jcol)
         END DO
        END IF
       END DO
c-----------------------------------------------------------------------
c     Update number of user-defined regressors groups
c-----------------------------------------------------------------------
       Nguhl=Nguhl-idel
c-----------------------------------------------------------------------
c     Estimate reduced model
c-----------------------------------------------------------------------
       CALL regvar(Trnsrs,Nobspf,Fctdrp,Nfcst,0,Userx,Bgusrx,Nrusrx,
     &             Priadj,Reglom,Nrxy,Begxy,Frstry,T,Elong)
       IF(.not.Lfatal)CALL rgarma(T,Mxiter,Mxnlit,Lprtit,A,Na,Nefobs,
     &                            argok)
       IF((.not.Lfatal).and.(Lautom.or.Lautox).and.(.not.argok))Lester=T
c-----------------------------------------------------------------------
c     if no groups removed, print message
c-----------------------------------------------------------------------
      ELSE
       IF(Lprchi)WRITE(Mt1,1030)
      END IF
c-----------------------------------------------------------------------
 1010 FORMAT(//,' User-defined Holiday Regression groups removed (at ',
     &            f12.6,' percent level):')
 1020 FORMAT(5x,a)
 1030 FORMAT(//,' No User-defined Holiday Regression groups removed.')
c-----------------------------------------------------------------------
      RETURN
      END
c-----------------------------------------------------------------------
