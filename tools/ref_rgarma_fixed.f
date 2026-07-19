C     ref_rgarma_fixed.f -- reference output for rgarma with a FIXED ARMA
C     coefficient (test-plan C2/B3). Same 24-point ARMA(1,1) as ref_rgarma but
C     theta is HELD at 0.4 (Arimaf(2)=.true.), so only phi is estimated: Nestpm=1,
C     lmdif runs over one parameter, and upespm/setmdl exercise the fixed-lag skip
C     that every all-free case bypasses. Golden dump: Convrg, Nliter, Nfev, phi
C     (Arimap(1)), theta (Arimap(2), must stay 0.4), Var, Lnlkhd.
C       gfortran -O2 -ffp-contract=off -Ioracle/fortran tools/ref_rgarma_fixed.f \
C         <same object list as ref_rgarma> -o ref_rgarma_fixed && ./ref_rgarma_fixed
      PROGRAM ref_rgfix
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

      Nspobs=24
      Ncxy=1
      DO i=1,24
       Xy(i)=ser(i)
      END DO

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
      Arimaf(2)=.true.
      Oprfac(1)=1
      Oprfac(2)=1
      Mxarlg=1
      Mxmalg=1
      Mxdflg=0
      Arimap(1)=0.3D0
      Arimap(2)=0.4D0

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

      WRITE(*,'(A,L2)') 'rg_convrg ',Convrg
      WRITE(*,'(A,I4)') 'rg_nliter ',Nliter
      WRITE(*,'(A,I4)') 'rg_nfev ',Nfev
      WRITE(*,'(A,I4)') 'rg_nestpm ',Nestpm
      WRITE(*,'(A,ES24.16)') 'rg_phi ',Arimap(1)
      WRITE(*,'(A,ES24.16)') 'rg_theta ',Arimap(2)
      WRITE(*,'(A,ES24.16)') 'rg_var ',Var
      WRITE(*,'(A,ES24.16)') 'rg_lnlkhd ',Lnlkhd
      END

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
