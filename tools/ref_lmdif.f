C     ref_lmdif.f -- reference output for lmdif (Census-modified MINPACK LM core).
C       gfortran -O2 -ffp-contract=off -Ioracle/fortran tools/ref_lmdif.f \
C         oracle/fortran/{lmdif,fdjac2,qrfac,qrsolv,lmpar,enorm,dpmpar,dpeq}.f \
C         -o ref_lmdif && ./ref_lmdif
C     upespm/prtitr are model-side; lmdif never reads back what they write and
C     Nprint=0 so prtitr is never called -- stubbed no-op below (matches the C++
C     empty sync/prtitr hooks).
      PROGRAM ref_lmdif
      IMPLICIT NONE
      DOUBLE PRECISION enorm
      EXTERNAL rosen,expfit,enorm
      DOUBLE PRECISION x(2),fvec(5),diag(2),fjac(5,2),qtf(2)
      DOUBLE PRECISION wa1(2),wa2(2),wa3(2),wa4(5)
      DOUBLE PRECISION ftol,xtol,gtol,epsfcn,factor,fnorm
      INTEGER ipvt(2),mxiter,mode,nprint,info,nliter,nfev,ldfjac,m,n
      LOGICAL F
      PARAMETER(F=.false.)

C     ================= Case A -- Rosenbrock, full convergence =================
      m=2
      n=2
      x(1)=-1.2D0
      x(2)=1.0D0
      ftol=1.0D-10
      xtol=1.0D-10
      gtol=0.0D0
      mxiter=100
      epsfcn=0.0D0
      mode=1
      factor=100.0D0
      nprint=0
      nliter=0
      nfev=0
      ldfjac=2
      CALL lmdif(rosen,m,n,x,fvec,F,F,ftol,xtol,gtol,mxiter,
     &           epsfcn,diag,mode,factor,nprint,info,nliter,
     &           nfev,fjac,ldfjac,ipvt,qtf,wa1,wa2,wa3,wa4)
      fnorm=enorm(m,fvec)
      WRITE(*,'(A,I6)')      'a_info ',info
      WRITE(*,'(A,I6)')      'a_nliter ',nliter
      WRITE(*,'(A,I6)')      'a_nfev ',nfev
      WRITE(*,'(A,ES24.16)') 'a_x1 ',x(1)
      WRITE(*,'(A,ES24.16)') 'a_x2 ',x(2)
      WRITE(*,'(A,ES24.16)') 'a_fnorm ',fnorm
      WRITE(*,'(A,ES24.16)') 'a_fj11 ',fjac(1,1)
      WRITE(*,'(A,ES24.16)') 'a_fj22 ',fjac(2,2)
      WRITE(*,'(A,ES24.16)') 'a_qtf1 ',qtf(1)
      WRITE(*,'(A,ES24.16)') 'a_qtf2 ',qtf(2)
      WRITE(*,'(A,I6)')      'a_ipvt1 ',ipvt(1)
      WRITE(*,'(A,I6)')      'a_ipvt2 ',ipvt(2)
      WRITE(*,'(A,ES24.16)') 'a_diag1 ',diag(1)
      WRITE(*,'(A,ES24.16)') 'a_diag2 ',diag(2)

C     ============= Case B -- exponential fit, M>N, Mode=2 =====================
      m=5
      n=2
      x(1)=1.0D0
      x(2)=0.0D0
      ftol=1.0D-10
      xtol=1.0D-10
      gtol=1.0D-10
      mxiter=100
      epsfcn=0.0D0
      mode=2
      diag(1)=2.0D0
      diag(2)=0.5D0
      factor=100.0D0
      nprint=0
      nliter=0
      nfev=0
      ldfjac=5
      CALL lmdif(expfit,m,n,x,fvec,F,F,ftol,xtol,gtol,mxiter,
     &           epsfcn,diag,mode,factor,nprint,info,nliter,
     &           nfev,fjac,ldfjac,ipvt,qtf,wa1,wa2,wa3,wa4)
      fnorm=enorm(m,fvec)
      WRITE(*,'(A,I6)')      'b_info ',info
      WRITE(*,'(A,I6)')      'b_nliter ',nliter
      WRITE(*,'(A,I6)')      'b_nfev ',nfev
      WRITE(*,'(A,ES24.16)') 'b_x1 ',x(1)
      WRITE(*,'(A,ES24.16)') 'b_x2 ',x(2)
      WRITE(*,'(A,ES24.16)') 'b_fnorm ',fnorm
      WRITE(*,'(A,ES24.16)') 'b_fj11 ',fjac(1,1)
      WRITE(*,'(A,ES24.16)') 'b_fj22 ',fjac(2,2)
      WRITE(*,'(A,ES24.16)') 'b_qtf1 ',qtf(1)
      WRITE(*,'(A,ES24.16)') 'b_qtf2 ',qtf(2)
      WRITE(*,'(A,I6)')      'b_ipvt1 ',ipvt(1)
      WRITE(*,'(A,I6)')      'b_ipvt2 ',ipvt(2)

C     ===== Case C -- cumulative counters + Info=5 (re-entrant Rosenbrock) =====
      m=2
      n=2
      x(1)=-1.2D0
      x(2)=1.0D0
      ftol=1.0D-10
      xtol=1.0D-10
      gtol=0.0D0
      mxiter=10
      epsfcn=0.0D0
      mode=1
      factor=100.0D0
      nprint=0
      nliter=7
      nfev=13
      ldfjac=2
      CALL lmdif(rosen,m,n,x,fvec,F,F,ftol,xtol,gtol,mxiter,
     &           epsfcn,diag,mode,factor,nprint,info,nliter,
     &           nfev,fjac,ldfjac,ipvt,qtf,wa1,wa2,wa3,wa4)
      fnorm=enorm(m,fvec)
      WRITE(*,'(A,I6)')      'c_info ',info
      WRITE(*,'(A,I6)')      'c_nliter ',nliter
      WRITE(*,'(A,I6)')      'c_nfev ',nfev
      WRITE(*,'(A,ES24.16)') 'c_x1 ',x(1)
      WRITE(*,'(A,ES24.16)') 'c_x2 ',x(2)
      WRITE(*,'(A,ES24.16)') 'c_fnorm ',fnorm
      END

C     ---- Rosenbrock residuals: f1 = 10*(x2 - x1^2), f2 = 1 - x1 ----
      SUBROUTINE rosen(M,N,X,Fvec,Lauto,Gudrun,Iflag,Lckinv)
      IMPLICIT NONE
      INTEGER M,N,Iflag
      DOUBLE PRECISION X(N),Fvec(M)
      LOGICAL Lauto,Gudrun,Lckinv
      Fvec(1)=1.0D1*(X(2)-X(1)*X(1))
      Fvec(2)=1.0D0-X(1)
      RETURN
      END

C     ---- Overdetermined exponential fit: fi = x1*exp(x2*t_i) - y_i ----
      SUBROUTINE expfit(M,N,X,Fvec,Lauto,Gudrun,Iflag,Lckinv)
      IMPLICIT NONE
      INTEGER M,N,Iflag,i
      DOUBLE PRECISION X(N),Fvec(M),t(5),y(5)
      LOGICAL Lauto,Gudrun,Lckinv
      DATA t/0.5D0,1.0D0,1.5D0,2.0D0,2.5D0/
      DATA y/1.8D0,1.2D0,0.9D0,0.5D0,0.3D0/
      DO i=1,M
       Fvec(i)=X(1)*EXP(X(2)*t(i))-y(i)
      END DO
      RETURN
      END

C     ---- model-side stubs: lmdif never reads back what these write ----
      SUBROUTINE upespm(Estprm)
      IMPLICIT NONE
      DOUBLE PRECISION Estprm(*)
      RETURN
      END

      SUBROUTINE prtitr(A,Na,Parms,Nparms,Itrlbl,Iter,Nfev)
      IMPLICIT NONE
      CHARACTER Itrlbl*(*)
      INTEGER Na,Nparms,Iter,Nfev
      DOUBLE PRECISION A(*),Parms(Nparms)
      RETURN
      END
