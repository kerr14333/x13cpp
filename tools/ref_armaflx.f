C     ref_armaflx.f -- reference output for the full armafl (exact ARMA filter).
C     Builds an ARMA(1,1) model (phi=0.5, theta=0.3, no differencing) in the
C     model/mdldat commons and filters a length-8 series to residuals, with
C     Linit=T (initialize G'G, D, chol(var(w_p|z))) and Lckrts=T.
C       gfortran -O2 -ffp-contract=off -Ioracle/fortran tools/ref_armaflx.f \
C         oracle/fortran/{armafl,intgpg,exctma,chkrts,mltpos,ratpos,ratneg,copy,
C         setdp,scrmlt,dppfa,dsolve,logdet,dpmpar,ddot,daxpy,dpeq,uconv,xpand,
C         euclid,arflt,xprmx,maxlag}.f -o ref_armaflx && ./ref_armaflx
      PROGRAM ref_armaflx
      IMPLICIT NONE
      INCLUDE 'srslen.prm'
      INCLUDE 'model.prm'
      INCLUDE 'model.cmn'
      INCLUDE 'mdldat.cmn'
      INTEGER na,info,i
      DOUBLE PRECISION mata(200)

C     ---- ARMA(1,1): AR op (lag1, phi=0.5) then MA op (lag1, theta=0.3) ----
      Lar=.true.
      Lma=.true.
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
      Arimap(1)=0.5D0
      Arimap(2)=0.3D0
      Arimaf(1)=.false.
      Arimaf(2)=.false.
      Oprfac(1)=1
      Oprfac(2)=1
      Mxarlg=1
      Mxmalg=1
      Mxdflg=0
      Lndtcv=0D0

C     ---- length-8 series ----
      mata(1)=1.0D0
      mata(2)=2.0D0
      mata(3)=-1.0D0
      mata(4)=0.5D0
      mata(5)=3.0D0
      mata(6)=-2.0D0
      mata(7)=1.5D0
      mata(8)=0.25D0

      info=-99
      na=-99
      CALL armafl(8,1,.true.,.true.,mata,na,200,info)
      WRITE(*,'(A,I4)')      'af_info ',info
      WRITE(*,'(A,I4)')      'af_na ',na
      WRITE(*,'(A,ES24.16)') 'af_ldt ',Lndtcv
      DO i=1,na
       WRITE(*,'(A,I1,A,ES24.16)') 'af_a',i,' ',mata(i)
      END DO

C     ==== Case B: AR(2)MA(1) -- phi=(0.4,-0.2), theta=0.3; 2x2 D/Chlvwp ====
      Lar=.true.
      Lma=.true.
      Nopr=2
      Mdl(0)=1
      Mdl(1)=1
      Mdl(2)=2
      Mdl(3)=3
      Opr(0)=1
      Opr(1)=3
      Opr(2)=4
      Arimal(1)=1
      Arimal(2)=2
      Arimal(3)=1
      Arimap(1)=0.4D0
      Arimap(2)=-0.2D0
      Arimap(3)=0.3D0
      Arimaf(1)=.false.
      Arimaf(2)=.false.
      Arimaf(3)=.false.
      Oprfac(1)=1
      Oprfac(2)=1
      Mxarlg=2
      Mxmalg=1
      Mxdflg=0
      Lndtcv=0D0
      mata(1)=1.0D0
      mata(2)=2.0D0
      mata(3)=-1.0D0
      mata(4)=0.5D0
      mata(5)=3.0D0
      mata(6)=-2.0D0
      mata(7)=1.5D0
      mata(8)=0.25D0
      info=-99
      na=-99
      CALL armafl(8,1,.true.,.true.,mata,na,200,info)
      WRITE(*,'(A,I4)')      'bf_info ',info
      WRITE(*,'(A,I4)')      'bf_na ',na
      WRITE(*,'(A,ES24.16)') 'bf_ldt ',Lndtcv
      DO i=1,na
       WRITE(*,'(A,I2,A,ES24.16)') 'bf_a',i,' ',mata(i)
      END DO

C     ==== Case C: AIRLINE (0 1 1)(0 1 1)12 -- the production model ====
C     DIFF: reg (1-B) lag1 + seas (1-B^12) lag12, coef 1.0 fixed.
C     MA:   reg (1-0.6B) lag1 + seas (1-0.5B^12) lag12.
      Lar=.false.
      Lma=.true.
      Nopr=4
      Mdl(0)=1
      Mdl(1)=3
      Mdl(2)=3
      Mdl(3)=5
      Opr(0)=1
      Opr(1)=2
      Opr(2)=3
      Opr(3)=4
      Opr(4)=5
      Arimal(1)=1
      Arimal(2)=12
      Arimal(3)=1
      Arimal(4)=12
      Arimap(1)=1D0
      Arimap(2)=1D0
      Arimap(3)=0.6D0
      Arimap(4)=0.5D0
      Arimaf(1)=.true.
      Arimaf(2)=.true.
      Arimaf(3)=.false.
      Arimaf(4)=.false.
      Oprfac(1)=1
      Oprfac(2)=12
      Oprfac(3)=1
      Oprfac(4)=12
      Mxarlg=0
      Mxmalg=13
      Mxdflg=13
      Lndtcv=0D0
C     length-40 deterministic series (integer formula -> exact both sides)
      DO i=1,40
       mata(i)=DBLE(MOD(7*i,13))-6D0+0.25D0*DBLE(MOD(3*i,8))
      END DO
      info=-99
      na=-99
      CALL armafl(40,1,.true.,.true.,mata,na,200,info)
      WRITE(*,'(A,I4)')      'cf_info ',info
      WRITE(*,'(A,I4)')      'cf_na ',na
      WRITE(*,'(A,ES24.16)') 'cf_ldt ',Lndtcv
C     print a few residuals (first, middle, last) to keep output compact
      WRITE(*,'(A,ES24.16)') 'cf_a1 ',mata(1)
      WRITE(*,'(A,ES24.16)') 'cf_a2 ',mata(2)
      WRITE(*,'(A,ES24.16)') 'cf_a14 ',mata(14)
      WRITE(*,'(A,ES24.16)') 'cf_alast ',mata(na)

C     ==== Case D: Nopr=0 no-op -> Na=Nr, Mata untouched, Info=0 ====
      Nopr=0
      mata(1)=3.14D0
      mata(2)=-2.71D0
      info=-99
      na=-99
      CALL armafl(5,1,.true.,.true.,mata,na,200,info)
      WRITE(*,'(A,I4)') 'df_info ',info
      WRITE(*,'(A,I4)') 'df_na ',na
      WRITE(*,'(A,ES24.16)') 'df_a1 ',mata(1)
      END
