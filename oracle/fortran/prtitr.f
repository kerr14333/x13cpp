      SUBROUTINE prtitr(A,Na,Parms,Nparms,Itrlbl,Iter,Nfev)
      IMPLICIT NONE
c-----------------------------------------------------------------------
c     prtitr.f, Release 1, Subroutine Version 1.8, Modified 16 Feb 1995.
c-----------------------------------------------------------------------
c     Prints out the iteration, either the nonlinear or the
c overall iteration, the deviance, |G'G|**(1/nsrs)*a'a, and the
c parameter estimates.  Note that a'a must already have the determinate
c factored in.
c-----------------------------------------------------------------------
c Name   Type Description
c-----------------------------------------------------------------------
c a      d    Input either an array of exact likelihood values
c              a*|G'G|**(.5/nsrs) or its sum of squares
c dev    d    Local scalar for the deviance, the log likelihood without
c              constants, log(detcov)+sum(e(t)^2/v(t),t=1,nefobs)
c i      i    Local do loop index for write
c iter   i    Input number of iterations either overall or nonlinear
c              iterations
c itrlbl c    Input label to identify whether these are regression
c              parameters or ARMA nonlinear parameters
c na     i    Input number of a's to sum.  If na > 1 then the routine
c              is called from lmdif and the non linear parameters are
c              to be printed.  If na = 1 then the regression parmeters
c              are to be printed and the dev has already been summed.
c nfev   i    Input number of function evaluations
c nparms i    Input number of parmeters to print
c parms  d    Input parameters
c-----------------------------------------------------------------------
      INCLUDE 'notset.prm'
      INCLUDE 'srslen.prm'
      INCLUDE 'model.prm'
      INCLUDE 'series.cmn'
      INCLUDE 'units.cmn'
      INCLUDE 'tbllog.prm'
      INCLUDE 'tbllog.cmn'
      INCLUDE 'mdltbl.i'
      INCLUDE 'model.cmn'
      INCLUDE 'error.cmn'
c     ------------------------------------------------------------------
      LOGICAL T,F
      DOUBLE PRECISION ONE,PI,TWO,ZERO
      PARAMETER(T=.true.,F=.false.,ZERO=0D0,ONE=1D0,TWO=2D0,
     &          PI=3.14159265358979D0)
c     ------------------------------------------------------------------
c     Changed by BCM Feb 1996 to ensure iteration information is printed
c     in multiple runs.
c     ------------------------------------------------------------------
      LOGICAL Frstcl,Scndcl
      COMMON /lgiter / Frstcl,Scndcl
c     ------------------------------------------------------------------
      CHARACTER Itrlbl*(*)
      INTEGER i,Iter,Na,Nfev,Nparms,ovrlit
      DOUBLE PRECISION A(*),dev,lnlkhd,Parms(Nparms)
c     ------------------------------------------------------------------
      LOGICAL dpeq
      EXTERNAL dpeq
c-----------------------------------------------------------------------
      IF(Scndcl)THEN
       IF(Nb.eq.0)THEN
        WRITE(Mt1,1010)
 1010   FORMAT(/,' ARMA Iterations')
c     ------------------------------------------------------------------
       ELSE
        WRITE(Mt1,1020)
 1020   FORMAT(/,' Iterations',/,
     &'  IGLS:  Estimate regression parameters given last values of ARMA
     & parameters.',/,
     &'  ARMA:  Estimate ARMA parameters using residuals from last IGLS 
     &regression.',/,
     &'  NOTE:  ARMA iteration counts are cumulative over IGLS iteration
     &s.')
       END IF
c       Frstcl=F
      END IF
c-----------------------------------------------------------------------
c     Calculate the log likelihood from the deviance
c-----------------------------------------------------------------------
      IF(Na.gt.0)THEN
       IF(Na.gt.1)THEN
        CALL yprmy(A,Na,dev)
       ELSE
        dev=A(1)
       END IF
       IF(dpeq(dev,ZERO))THEN
        lnlkhd=ZERO
       ELSE
        lnlkhd=-Dnefob/TWO*(log(TWO*PI*dev/Dnefob)+ONE)
       END IF
c     ------------------------------------------------------------------
      END IF
c-----------------------------------------------------------------------
c     If printing out the parameters because of an error don't print the
c labels.  Print out the initial values on the first call and print 
c the iteration headers without a trailing blank line on the second
c call.  Only print the initial log likelihood on pure ARMA models.
c-----------------------------------------------------------------------
      IF(Iter.ne.NOTSET)THEN
       IF(Frstcl)THEN
        Scndcl=T
c     ------------------------------------------------------------------
        IF(Nb.eq.0)THEN
         WRITE(Mt1,1030)' ARMA parameters'
 1030    FORMAT(/,' Initial values for the',a)
         WRITE(Mt1,1040)lnlkhd
 1040    FORMAT('  Log Likelihood',1p,e23.9)
        ELSE
         WRITE(Mt1,1030)' '
        END IF
c     ------------------------------------------------------------------
       ELSE
        IF(.not.Scndcl)THEN
         WRITE(Mt1,'(1x)')
        ELSE IF(.not.Frstcl)THEN
         Scndcl=F
        END IF
c     ------------------------------------------------------------------
        IF(Nb.eq.0)THEN
         WRITE(Mt1,1050)Iter
 1050    FORMAT('  ','Iteration',t30,i10)
c     ------------------------------------------------------------------
        ELSE IF(Itrlbl.eq.'IGLS')THEN
         WRITE(Mt1,1060)Itrlbl,Iter
 1060    FORMAT(/,' ',a,' Iteration',t30,i10)
        ELSE
         WRITE(Mt1,1070)Itrlbl,Iter
 1070    FORMAT('  ',a,' Iteration',t30,i10)
        END IF
c     ------------------------------------------------------------------
        WRITE(Mt1,1080)Nfev
 1080   FORMAT('  Function evaluations',t30,i10)
c     ------------------------------------------------------------------
        WRITE(Mt1,1040)lnlkhd
       END IF
      END IF
c     ------------------------------------------------------------------
      IF(Itrlbl.eq.'IGLS')THEN
       WRITE(Mt1,1090)'Regression',(Parms(i),i=1,Nparms)
 1090  FORMAT('  ',a,' parameters',t25,3g23.9,/,(t22,3g23.9))
       ovrlit=Iter
c     ------------------------------------------------------------------
      ELSE
       WRITE(Mt1,1090)'ARMA',(Parms(i),i=1,Nparms)
       IF(Savtab(LESTIT))THEN
        CALL savitr(F,ovrlit,Iter,lnlkhd,Parms,Nparms)
        IF(Lfatal)RETURN
       END IF
      END IF
c     ------------------------------------------------------------------
      Frstcl=F
c     ------------------------------------------------------------------
      RETURN
      END
