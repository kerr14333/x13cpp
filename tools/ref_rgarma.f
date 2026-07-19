C     ref_rgarma.f -- reference output for rgarma (THE regARIMA IGLS estimation
C     engine). No-regression (Nb=0) pure ARMA(1,1): phi & theta both free, exact
C     ML, on a 24-point stationary working series held in Xy (Ncxy=1). Lprier=F
C     and Lprtit=F so every diagnostic/iteration print is skipped (matching the
C     C++ deferral); Savtab(LESTIT)=F so savitr is not called; Issap=Irev=0 so
C     gudrun=T. Estimation runs a single IGLS pass (Nb=0) driving lmdif over the
C     two ARMA parameters, then builds the ARMA covariance (fdjac2/qrfac/covar).
C     Golden dump: Armaer, Convrg, Nliter, Nfev, na, nefobs, the estimated
C     coefficients (Arimap after upespm), Var, Lnlkhd, and the covariance diag.
C       gfortran -O2 -ffp-contract=off -Ioracle/fortran tools/ref_rgarma.f \
C         oracle/fortran/{rgarma,strtvl,setmdl,upespm,roots,revrse,rpoly,fxshfr,\
C         quadit,realit,calcsc,nextk,newest,quadsd,quad,chkrt2,olsreg,resid,\
C         yprmy,maxvec,stpitr,fcnar,armafl,intgpg,exctma,chkrts,mltpos,ratpos,\
C         ratneg,arflt,maxlag,lmdif,fdjac2,qrfac,qrsolv,lmpar,enorm,covar,copy,\
C         dcopy,setdp,scrmlt,dppfa,dsolve,logdet,dpmpar,ddot,daxpy,dpeq,uconv,\
C         xpand,euclid,xprmx}.f -o ref_rgarma && ./ref_rgarma
      PROGRAM ref_rgarma
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
      INTEGER na,nefobs,i
      LOGICAL lauto
      DOUBLE PRECISION a(1092)

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
C     ---- starting values: phi0=0.3, theta0=0.3 ----
      Arimap(1)=0.3D0
      Arimap(2)=0.3D0

C     ---- no regression, no differencing intervals ----
      Nb=0
      Nintvl=0
      Nextvl=0

C     ---- tolerances / step (overall Tol used since Nb=0) ----
      Tol=1D-5
      Nltol0=1D-5
      Nltol=1D-5
      Stepln=0D0

C     ---- run controls ----
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

      WRITE(*,'(A,I4)') 'rg_armaer ',Armaer
      WRITE(*,'(A,L2)') 'rg_convrg ',Convrg
      WRITE(*,'(A,I4)') 'rg_nliter ',Nliter
      WRITE(*,'(A,I4)') 'rg_nfev ',Nfev
      WRITE(*,'(A,I4)') 'rg_na ',na
      WRITE(*,'(A,I4)') 'rg_nefobs ',nefobs
      WRITE(*,'(A,L2)') 'rg_lauto ',lauto
      WRITE(*,'(A,L2)') 'rg_lcalcm ',Lcalcm
      WRITE(*,'(A,ES24.16)') 'rg_phi ',Arimap(1)
      WRITE(*,'(A,ES24.16)') 'rg_theta ',Arimap(2)
      WRITE(*,'(A,ES24.16)') 'rg_var ',Var
      WRITE(*,'(A,ES24.16)') 'rg_lnlkhd ',Lnlkhd
      WRITE(*,'(A,ES24.16)') 'rg_ldtcv ',Lndtcv
      WRITE(*,'(A,ES24.16)') 'rg_cm11 ',Armacm(1,1)
      WRITE(*,'(A,ES24.16)') 'rg_cm22 ',Armacm(2,2)
      END

C     ---- stubs: output/error paths unreachable with Lprier=F, Lprtit=F, no
C     errors, Savtab(LESTIT)=F ----
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
