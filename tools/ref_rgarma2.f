C     ref_rgarma2.f -- reference output for rgarma WITH regression (Nb>0). Same
C     24-point series as ref_rgarma.f but with mean 2.0 and an intercept column,
C     so Xy is 24x2 = [1, y] per row (Ncxy=2, Nb=1). This drives the branches the
C     Nb=0 case skips: the olsreg GLS solve (+ Nfev+=Ncxy+1 per pass), the
C     multi-pass IGLS outer loop (locest stays true while Nb>0, tnltol switches
C     from 2/n*Nltol0 to 2/n*Nltol after iter 2), and resid over a real
C     regression column. ARMA(1,1) both free, exact ML, Lprier/Lprtit=F.
C       gfortran -O2 -ffp-contract=off -Ioracle/fortran tools/ref_rgarma2.f \
C         oracle/fortran/{rgarma,strtvl,setmdl,upespm,roots,revrse,rpoly,fxshfr,\
C         quadit,realit,calcsc,nextk,newest,quadsd,quad,chkrt2,olsreg,resid,\
C         yprmy,maxvec,stpitr,fcnar,armafl,intgpg,exctma,chkrts,mltpos,ratpos,\
C         ratneg,arflt,maxlag,lmdif,fdjac2,qrfac,qrsolv,lmpar,enorm,covar,copy,\
C         dcopy,setdp,scrmlt,dppfa,dsolve,logdet,dpmpar,ddot,daxpy,dpeq,uconv,\
C         xpand,euclid,xprmx}.f -o ref_rgarma2 && ./ref_rgarma2
      PROGRAM ref_rgarma2
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
      DOUBLE PRECISION a(1092),ser(24)
      DATA ser/0.5D0,-0.3D0,0.8D0,-0.6D0,0.2D0,0.9D0,-0.7D0,0.4D0,
     &         0.1D0,-0.5D0,0.6D0,-0.2D0,0.7D0,-0.8D0,0.3D0,0.5D0,
     &         -0.4D0,0.9D0,-0.1D0,0.6D0,-0.7D0,0.2D0,0.4D0,-0.5D0/

C     ---- [X:y] with an intercept: row i = [1, ser(i)+2], Ncxy=2, Nb=1 ----
      Nspobs=24
      Ncxy=2
      DO i=1,24
       Xy(2*i-1)=1D0
       Xy(2*i)=ser(i)+2D0
      END DO

C     ---- ARMA(1,1) model shell ----
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

C     ---- one regressor, no differencing intervals ----
      Nb=1
      Nintvl=0
      Nextvl=0

C     ---- tolerances / step ----
      Tol=1D-5
      Nltol0=1D-3
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
      WRITE(*,'(A,ES24.16)') 'rg_b1 ',B(1)
      WRITE(*,'(A,ES24.16)') 'rg_phi ',Arimap(1)
      WRITE(*,'(A,ES24.16)') 'rg_theta ',Arimap(2)
      WRITE(*,'(A,ES24.16)') 'rg_var ',Var
      WRITE(*,'(A,ES24.16)') 'rg_lnlkhd ',Lnlkhd
      WRITE(*,'(A,ES24.16)') 'rg_ldtcv ',Lndtcv
      WRITE(*,'(A,ES24.16)') 'rg_cm11 ',Armacm(1,1)
      WRITE(*,'(A,ES24.16)') 'rg_cm22 ',Armacm(2,2)
      END

C     ---- stubs (unreachable output/error paths with Lprier=F, Lprtit=F) ----
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
