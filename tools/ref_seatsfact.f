C     ref_seatsfact.f -- reference (oracle) output for PARFRA + MAK1.
C       python tools/extract_seatsfact.py
C       gfortran -O2 -ffp-contract=off -Ioracle/fortran tools/ref_seatsfact.f \
C         tools/ref_seatsfact_snip.f oracle/fortran/dpeq.f -o ref_seatsfact \
C         && ./ref_seatsfact
C     Bits printed ES24.16 (17 sig digits -> round-trips to the exact double).
C     The extracted snippet is the *oracle* Fortran verbatim, so these are the
C     oracle's exact bits for the C++ parity gate.
      PROGRAM ref_seatsfact
      IMPLICIT NONE
C     Commons touched by the linked routines.
      INTEGER NIO,Nprof
      COMMON /stream/ NIO,Nprof
      REAL*8 XL
      COMMON /unitmak/ XL
      LOGICAL Lfatal
      COMMON /fcnerr/ Lfatal
      INTEGER Mt,Mtm,Mt1,Mt2,Nform,Ng,Mtprof
      COMMON /units / Mt,Mtm,Mt2,Mt1,Ng,Nform,Mtprof

      REAL*8 rt(60),t(60),s(60),u(60),v(60)
      REAL*8 ufin(64),theta(64),var,toterr
      INTEGER nrt,nt,ns,nu,nv,nufin,ntheta,i
      CHARACTER cap*60

      NIO=6
      Nprof=0
      Ng=6
      Lfatal=.FALSE.
      XL=0.99D0
      cap=' '

C     ================= PARFRA case 1 =================
C     rt/(t*s) = u/t + v/s ; t,s harmonic funcs (powers of cos w).
      nt=3
      t(1)=2.0D0
      t(2)=-1.0D0
      t(3)=0.5D0
      ns=3
      s(1)=3.0D0
      s(2)=0.4D0
      s(3)=-0.2D0
      nrt=4
      rt(1)=1.0D0
      rt(2)=2.0D0
      rt(3)=3.0D0
      rt(4)=4.0D0
      CALL PARFRA(rt,nrt,t,nt,s,ns,u,nu,v,nv)
      WRITE(*,'(A,I6)') 'pf1_nu ',nu
      WRITE(*,'(A,I6)') 'pf1_nv ',nv
      DO i=1,nu
       WRITE(*,'(A,I1,A,ES24.16)') 'pf1_u',i,' ',u(i)
      END DO
      DO i=1,nv
       WRITE(*,'(A,I1,A,ES24.16)') 'pf1_v',i,' ',v(i)
      END DO

C     ================= MAK1 case A (nufin=2, degenerate n<=2) =========
C     ACF of MA(1) theta=1-0.5B, sigma^2=1: gam0=1.25, gam1=-0.5.
      nufin=2
      ufin(1)=1.25D0
      ufin(2)=-1.0D0
      CALL MAK1(ufin,nufin,theta,ntheta,var,0,1,cap,0,toterr)
      WRITE(*,'(A,I6)') 'mkA_nt ',ntheta
      DO i=1,ntheta
       WRITE(*,'(A,I1,A,ES24.16)') 'mkA_th',i,' ',theta(i)
      END DO
      WRITE(*,'(A,ES24.16)') 'mkA_var  ',var
      WRITE(*,'(A,ES24.16)') 'mkA_terr ',toterr

C     ================= MAK1 case B (nufin=3, real roots) =============
C     ACF of MA(2) theta=(1,-0.5,0.2): gam0=1.29, gam1=-0.6, gam2=0.2.
      nufin=3
      ufin(1)=1.29D0
      ufin(2)=-1.2D0
      ufin(3)=0.4D0
      CALL MAK1(ufin,nufin,theta,ntheta,var,0,1,cap,0,toterr)
      WRITE(*,'(A,I6)') 'mkB_nt ',ntheta
      DO i=1,ntheta
       WRITE(*,'(A,I1,A,ES24.16)') 'mkB_th',i,' ',theta(i)
      END DO
      WRITE(*,'(A,ES24.16)') 'mkB_var  ',var
      WRITE(*,'(A,ES24.16)') 'mkB_terr ',toterr

C     ================= MAK1 case C (nufin=3, complex roots) ==========
C     ACF of MA(2) theta=(1,0,0.25) (roots +-2i): gam0=1.0625,g1=0,g2=0.25.
      nufin=3
      ufin(1)=1.0625D0
      ufin(2)=0.0D0
      ufin(3)=0.5D0
      CALL MAK1(ufin,nufin,theta,ntheta,var,0,1,cap,0,toterr)
      WRITE(*,'(A,I6)') 'mkC_nt ',ntheta
      DO i=1,ntheta
       WRITE(*,'(A,I1,A,ES24.16)') 'mkC_th',i,' ',theta(i)
      END DO
      WRITE(*,'(A,ES24.16)') 'mkC_var  ',var
      WRITE(*,'(A,ES24.16)') 'mkC_terr ',toterr
      END
C     OutRPQ is guarded off (noprint=1) in every call here -- never executed.
      subroutine OutRPQ(Nio,nroots,rez,imz,m,ar,p)
      integer Nio,nroots
      real*8 rez(*),imz(*),m(*),ar(*),p(*)
      end
