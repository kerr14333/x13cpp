C     ref_setmdl.f -- reference output for setmdl (pack estprm + root check).
C     MA(1) with theta=0.95 (root modulus ~1.0526). Three calls on one model
C     exercise the SAVEd `first`: call 1 (first=T, no shrink), call 2 (first=F,
C     near-unit-circle root <=1.06 -> shrink by 0.9**lag to 0.855), call 3 (root
C     now ~1.169, no shrink). All invertible, so the getstr/WRITE/abend branches
C     are never executed (matching the C++ deferral of that I/O).
C       gfortran -O2 -ffp-contract=off -Ioracle/fortran tools/ref_setmdl.f \
C         oracle/fortran/{setmdl,roots,revrse,rpoly,fxshfr,quadit,realit,calcsc,\
C         nextk,newest,quadsd,quad,setdp,dpeq,getstr,abend}.f -o ref_setmdl
      PROGRAM ref_setmdl
      IMPLICIT NONE
      INCLUDE 'stdio.i'
      INCLUDE 'srslen.prm'
      INCLUDE 'model.prm'
      INCLUDE 'model.cmn'
      INCLUDE 'mdldat.cmn'
      DOUBLE PRECISION estprm(PARIMA)
      LOGICAL laumts
      INTEGER icall,ilaum

C     ---- MA(1) model shell: DIFF empty, AR empty, MA = operator 1, lag 1 ----
      Mdl(0)=1
      Mdl(1)=1
      Mdl(2)=1
      Mdl(3)=2
      Opr(0)=1
      Opr(1)=2
      Arimal(1)=1
      Oprfac(1)=1
      Arimap(1)=0.95D0
      Arimaf(1)=.false.
      Lar=.false.

      DO icall=1,3
       laumts=.false.
       CALL setmdl(estprm,laumts)
       ilaum=0
       IF(laumts)ilaum=1
       WRITE(*,'(A,I1,A,I6)')      'c',icall,'_nestpm ',Nestpm
       WRITE(*,'(A,I1,A,I6)')      'c',icall,'_laum ',ilaum
       WRITE(*,'(A,I1,A,ES24.16)') 'c',icall,'_ep1 ',estprm(1)
       WRITE(*,'(A,I1,A,ES24.16)') 'c',icall,'_ap1 ',Arimap(1)
      END DO
      END

C     ---- stubs for the deferred error-path routines (never executed on the
C     valid path exercised here) ----
      SUBROUTINE getstr(Titles,Ptr,Nptr,Idx,Str,Nchar)
      IMPLICIT NONE
      CHARACTER Titles*(*),Str*(*)
      INTEGER Ptr(*),Nptr,Idx,Nchar
      Nchar=0
      RETURN
      END

      SUBROUTINE abend
      IMPLICIT NONE
      RETURN
      END
