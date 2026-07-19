C     ref_fcstxy.f -- reference output for fcstxy (regARIMA forecasts + forecast
C     standard errors). Estimates the same no-regression ARMA(1,1) as ref_rgarma
C     (24-point series, Nb=0, no differencing so Fctori=Nspobs=24), then forecasts
C     6 steps ahead. With Nb=0 the design-uncertainty term is skipped (Rgvar=0),
C     so this validates the forecast recursion + the psi(B)-weight standard
C     errors. Golden dump: Fcst(1..6), Se(1..6), Rgvar(1..6).
C       gfortran -O2 -ffp-contract=off -Ioracle/fortran tools/ref_fcstxy.f \
C         oracle/fortran/{rgarma,strtvl,setmdl,upespm,roots,revrse,rpoly,fxshfr,\
C         quadit,realit,calcsc,nextk,newest,quadsd,quad,chkrt2,olsreg,resid,\
C         yprmy,maxvec,stpitr,fcnar,armafl,intgpg,exctma,chkrts,mltpos,ratpos,\
C         ratneg,arflt,maxlag,lmdif,fdjac2,qrfac,qrsolv,lmpar,enorm,covar,copy,\
C         dcopy,setdp,scrmlt,dppfa,dsolve,logdet,dpmpar,ddot,daxpy,dpeq,uconv,\
C         xpand,euclid,xprmx,fcstxy,polyml,eltlen,dppsl}.f -o ref_fcstxy \
C         && ./ref_fcstxy
      PROGRAM ref_fcstxy
      IMPLICIT NONE
      INCLUDE 'srslen.prm'
      INCLUDE 'model.prm'
      INCLUDE 'mdltbl.i'
      INCLUDE 'tbllog.prm'
      INCLUDE 'tbllog.cmn'
      INCLUDE 'hiddn.cmn'
      INCLUDE 'series.cmn'
      INCLUDE 'model.cmn'
      INCLUDE 'mdldat.cmn'
      INCLUDE 'units.cmn'
      INCLUDE 'error.cmn'
      INTEGER na,nefobs,i,nfcst,fctori
      LOGICAL lauto
      DOUBLE PRECISION a(1092),fcst(6),se(6),rgvar(6)

C     ---- 24-point stationary working series (the y column of Xy, Ncxy=1) ----
      Nspobs=24
      Ncxy=1
      Xy(1)=0.5D0
      Xy(2)=-0.3D0
      Xy(3)=0.8D0
      Xy(4)=-0.6D0
      Xy(5)=0.2D0
      Xy(6)=0.9D0
      Xy(7)=-0.7D0
      Xy(8)=0.4D0
      Xy(9)=0.1D0
      Xy(10)=-0.5D0
      Xy(11)=0.6D0
      Xy(12)=-0.2D0
      Xy(13)=0.7D0
      Xy(14)=-0.8D0
      Xy(15)=0.3D0
      Xy(16)=0.5D0
      Xy(17)=-0.4D0
      Xy(18)=0.9D0
      Xy(19)=-0.1D0
      Xy(20)=0.6D0
      Xy(21)=-0.7D0
      Xy(22)=0.2D0
      Xy(23)=0.4D0
      Xy(24)=-0.5D0

C     ---- ARMA(1,1) model shell: one AR operator (lag 1), one MA operator ----
      Lar=.true.
      Lma=.true.
      Lextma=.true.
      Lextar=.false.
      Lprier=.false.
      Lhiddn=.false.
      Nopr=2
      Mdl(0)=1
      Mdl(1)=1
      Mdl(2)=2
      Mdl(3)=3
      Opr(0)=1
      Opr(1)=2
      Opr(2)=3
      Arimal(1)=1
      Arimal(2)=1
      Arimaf(1)=.false.
      Arimaf(2)=.false.
      Oprfac(1)=1
      Oprfac(2)=1
      Mxarlg=1
      Mxmalg=1
      Mxdflg=0
      Arimap(1)=0.3D0
      Arimap(2)=0.3D0

      Nb=0
      Nintvl=0
      Nextvl=0
      Iregfx=0

      Tol=1D-5
      Nltol0=1D-5
      Nltol=1D-5
      Stepln=0D0

      Issap=0
      Irev=0
      Lfatal=.false.
      Mt1=6
      Mt2=6
      DO i=1,NTBL
       Savtab(i)=.false.
      END DO

      lauto=.false.
      na=0
      nefobs=0
      CALL rgarma(.true.,200,60,.false.,a,na,nefobs,lauto)

C     ---- forecast 6 steps ahead (Fctori = undifferenced length = Nspobs) ----
      fctori=Nspobs
      nfcst=6
      CALL fcstxy(fctori,nfcst,fcst,se,rgvar)

      DO i=1,6
       WRITE(*,'(A,I1,A,ES24.16)') 'fc_fcst',i,' ',fcst(i)
      END DO
      DO i=1,6
       WRITE(*,'(A,I1,A,ES24.16)') 'fc_se',i,' ',se(i)
      END DO
      DO i=1,6
       WRITE(*,'(A,I1,A,ES24.16)') 'fc_rgvar',i,' ',rgvar(i)
      END DO
      END

C     ---- stubs (unreachable with Lprier=F, Lprtit=F, no errors) ----
      SUBROUTINE errhdr
      RETURN
      END
      SUBROUTINE abend
      RETURN
      END
      SUBROUTINE writln(a,b,c,d)
      CHARACTER a
      INTEGER b,c
      LOGICAL d
      RETURN
      END
      SUBROUTINE getstr(a,b,c,d,e,f)
      INTEGER b,c,d,f
      CHARACTER a,e
      RETURN
      END
      SUBROUTINE prtitr(a,b,c,d,e,f,g)
      INTEGER b,d,f,g
      DOUBLE PRECISION a,c
      CHARACTER e
      RETURN
      END
      SUBROUTINE savitr(a,b,c,d,e,f)
      LOGICAL a
      INTEGER b,c,f
      DOUBLE PRECISION d,e
      RETURN
      END
