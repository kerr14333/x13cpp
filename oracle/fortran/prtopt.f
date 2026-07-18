C     Last change:  BCM   5 May 1998    4:00 pm
      SUBROUTINE prtopt(Lestim,Mxiter,Mxnlit)
      IMPLICIT NONE
c-----------------------------------------------------------------------
c     prtopt.f, Release 1, Subroutine Version 1.5, Modified 16 Feb 1995.
c-----------------------------------------------------------------------
c     Prints the nonlinear estimation options
c-----------------------------------------------------------------------
      INCLUDE 'srslen.prm'
      INCLUDE 'model.prm'
      INCLUDE 'model.cmn'
      INCLUDE 'units.cmn'
c-----------------------------------------------------------------------
      CHARACTER cexact*(24),ceval*(40)
      LOGICAL Lestim
      INTEGER Mxiter,Mxnlit,nexact,neval
c-----------------------------------------------------------------------
      LOGICAL dpeq
      EXTERNAL dpeq
c-----------------------------------------------------------------------
      IF(Lextar.and.Lextma)THEN
       cexact='Exact ARMA'
       nexact=10
      ELSE IF(Lextma)THEN
       cexact='Exact MA, conditional AR'
       nexact=24
      ELSE
       cexact='Conditional'
       nexact=11
      END IF
c-----------------------------------------------------------------------
      IF(Lestim)THEN
       ceval='estimation'
       neval=10
      ELSE IF(Iregfx.gt.0)THEN
       ceval='evaluation'
       neval=10
      ELSE IF(Ncxy.gt.1)THEN
       ceval='evaluation with GLS regression estimates'
       neval=40
      ELSE
       ceval='evaluation'
       neval=10
      END IF
      WRITE(Mt1,1010)cexact(1:nexact),ceval(1:neval)
 1010 FORMAT('  ',a,' likelihood ',a)
c-----------------------------------------------------------------------
      IF(Lestim)THEN
       IF(Ncxy.gt.1)THEN
        WRITE(Mt1,1020)Mxiter
 1020   FORMAT('  Max total ARMA iterations     ',t39,i8)
        IF(Mxnlit.gt.0)WRITE(Mt1,1030)Mxnlit
 1030   FORMAT('  Max ARMA iter''s w/in an IGLS iteration   ',t39,i8)
        WRITE(Mt1,1040)Tol
 1040   FORMAT('  Convergence tolerance  ',t38,1p,g9.2)
        IF((.not.dpeq(Nltol,Tol)).OR.(.not.dpeq(Nltol0,100D0*Nltol)))
     &     WRITE(Mt1,1050)Nltol
 1050   FORMAT('  ARMA convergence tolerance',t38,1p,g9.2)
c-----------------------------------------------------------------------
       ELSE
        IF(Mxnlit.gt.0)WRITE(Mt1,1020)Mxiter
        WRITE(Mt1,1040)Tol
       END IF
      END IF
c-----------------------------------------------------------------------
      RETURN
      END
