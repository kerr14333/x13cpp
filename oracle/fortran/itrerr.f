C     Last change:  BCM  10 Feb 1999    4:06 pm
      SUBROUTINE itrerr(Errstr,Lauto,Issap,Irev)
      IMPLICIT NONE
c     ------------------------------------------------------------------
c     This subroutine prints out an error message if the number of
c     iterations or function evaluations is too large.  
c     ------------------------------------------------------------------
      LOGICAL T,F
      PARAMETER(T=.true.,F=.false.)
c     ------------------------------------------------------------------
      INCLUDE 'units.cmn'
      INCLUDE 'stdio.i'
c     ------------------------------------------------------------------
      CHARACTER Errstr*(*)
      LOGICAL Lauto,lparma
      INTEGER Issap,Irev
c     ------------------------------------------------------------------
      lparma=F
      IF(.not.Lauto)WRITE(Mt1,1010)
      WRITE(Mt2,1010)
 1010 FORMAT(/,
     &' ****************************************************************
     &*******')
      IF(Issap.eq.2)THEN
       IF(.not.Lauto)THEN
        WRITE(STDERR,1020)Errstr
        WRITE(Mt1,1020)Errstr
       END IF
       WRITE(Mt2,1020)Errstr
 1020  FORMAT(/,' ERROR: Estimation failed to converge -- maximum ',a,
     &          ' reached',/,'        during sliding spans analysis.')
      ELSE IF(Irev.eq.4)THEN
       IF(.not.Lauto)THEN
        WRITE(STDERR,1030)Errstr
        WRITE(Mt1,1030)Errstr
       END IF
       WRITE(Mt2,1030)Errstr
 1030  FORMAT(/,' ERROR: Estimation failed to converge -- maximum ',a,
     &          ' reached',/,'        during history analysis.')
      ELSE
       IF(.not.Lauto)THEN
        WRITE(STDERR,1040)Errstr
        WRITE(Mt1,1040)Errstr
       END IF
       WRITE(Mt2,1040)Errstr
 1040  FORMAT(/,' ERROR: Estimation failed to converge -- maximum ',a,
     &          ' reached.')
      END IF
      IF(.not.Lauto.and.Issap.lt.2.and.Irev.lt.4)WRITE(Mt1,1050)
 1050 FORMAT(/,'        Parameter values and log likelihood at ',
     &       'last iteration follow.',//)
      IF(.not.Lauto)WRITE(Mt1,1060)
      WRITE(Mt2,1060)
 1060 FORMAT('        Rerun program trying one of the following:',/,
     &       10x,'(1) Allow more iterations (set a larger value of ',
     &       'maxiter).')
      IF(Lauto)THEN
       WRITE(Mt2,1070)MDLSEC,PRGNAM,DOCNAM
 1070  FORMAT(10x,'(2) Try a different model.',//,1x,'See ',a,
     &        ' of the ',a,' ',a,' for more discussion.')
       WRITE(Mt2,1010)
      ELSE
       IF(Issap.eq.2.or.Irev.eq.4)THEN
        WRITE(Mt1,1080)
        WRITE(Mt2,1080)
 1080   FORMAT(10x,'(2) Fix the values of the ARMA coefficients to ',
     &         'those obtained',/,14x,
     &         'while estimating the full series (set fixmdl=yes)')
       ELSE
        WRITE(Mt1,1090)'in the log file'
        WRITE(Mt2,1090)'below'
 1090   FORMAT(10x,'(2) Use initial values for ARMA parameters as ',
     &         'given ',a,'.')
        lparma=T
       END IF
       WRITE(Mt1,1100)MDLSEC,PRGNAM,DOCNAM
       WRITE(Mt2,1100)MDLSEC,PRGNAM,DOCNAM
 1100  FORMAT(10x,'(3) Try a different model.',//,1x,'See ',a,
     &        ' of the ',a,' ',a,' for more discussion.')
c     ------------------------------------------------------------------
       IF(lparma)THEN
        WRITE(Mt2,*)' '
        CALL prARMA(Mt2)
        WRITE(Mt2,*)' '
       END IF
c     ------------------------------------------------------------------
       WRITE(Mt1,1010)
       WRITE(Mt2,1010)
      END IF
c     ------------------------------------------------------------------
      RETURN
      END
