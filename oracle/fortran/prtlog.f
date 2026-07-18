C     Last change:  BCM  26 Jan 98    1:50 pm
      SUBROUTINE prtlog(Ng,Insrs,Outsrs,Nopen,Unopnd,Nfail,Failed,
     &                  Mtafil,Logfil)
      IMPLICIT NONE
C-----------------------------------------------------------------------
c     Print out summary error messages into log file.
C-----------------------------------------------------------------------
      INCLUDE 'stdio.i'
C-----------------------------------------------------------------------
      LOGICAL T,F
      PARAMETER(T=.true.,F=.false.)
C-----------------------------------------------------------------------
      LOGICAL lhdr,Lexist
      CHARACTER Insrs*(PFILCR),Outsrs*(PFILCR),Mtafil*(*),Logfil*(*)
      INTEGER i,n1,n2,Ng,Nopen,Unopnd,Nfail,Failed
      DIMENSION Insrs(*),Outsrs(*),Unopnd(*),Failed(*)
C-----------------------------------------------------------------------
      INTEGER nblank
      EXTERNAL nblank
c-----------------------------------------------------------------------
      IF(Nopen.gt.0.or.Nfail.gt.0)THEN
       WRITE(Ng,1010)Mtafil
 1010  FORMAT(' Error messages for the input files defined in ',a)
       WRITE(STDERR,1020)Logfil,Mtafil
 1020  FORMAT(//,'   Check ',a,' to see which input files defined ',
     &        'in ',a,/,'   were terminated due to errors.')
      END IF
C-----------------------------------------------------------------------
      IF(Nfail.gt.0)THEN
       lhdr=T
C-----------------------------------------------------------------------
       DO i=1,Nfail
        n1=nblank(Insrs(Failed(i)))
        n2=nblank(Outsrs(Failed(i)))
        IF(n1.gt.0.and.n2.gt.0)THEN
         IF(lhdr)THEN
          WRITE(Ng,1030)
 1030     FORMAT(///,'  Input or runtime errors were found in the ',
     &               'following files:')
          lhdr=F
         END IF
         INQUIRE(FILE=Outsrs(Failed(i))(1:n2)//'.err',
     &           EXIST=Lexist)
         IF(Lexist)THEN
          WRITE(Ng,1040)Insrs(Failed(i))(1:n1),Outsrs(Failed(i))(1:n2)
 1040     FORMAT(5x,a,'.spc  (Error messages stored in ',a,'.err)')
         ELSE
          WRITE(Ng,1050)Insrs(Failed(i))(1:n1)
 1050     FORMAT(5x,a,'.spc')
         END IF
        END IF
       END DO
C-----------------------------------------------------------------------
      END IF
C-----------------------------------------------------------------------
      IF(Nopen.gt.0)THEN
       WRITE(Ng,1060)PRGNAM
 1060  FORMAT(///,'  ',a,' is unable to open input/output files ',
     &        'for the following sets of filenames:')
C-----------------------------------------------------------------------
       DO i=1,Nopen
        n1=nblank(Insrs(Unopnd(i)))
        n2=nblank(Outsrs(Unopnd(i)))
        IF(n1.gt.0.and.n2.gt.0)THEN
         WRITE(Ng,1070)i,Insrs(Unopnd(i))(1:n1),Outsrs(Unopnd(i))(1:n2)
 1070    FORMAT(2x,i3,2x,'Input filename:  ',a,/,
     &                7x,'Output filename: ',a)
        ELSE IF(n1.eq.0.and.n2.eq.0)THEN
         WRITE(Ng,1080)i
 1080    FORMAT(2x,i3,2x,'Input filename:  NOT SPECIFIED',/,
     &                7x,'Output filename: NOT SPECIFIED')
        ELSE IF(n1.eq.0)THEN
         WRITE(Ng,1090)i,Outsrs(Unopnd(i))(1:n2)
 1090    FORMAT(2x,i3,2x,'Input filename:  NOT SPECIFIED',/,
     &                7x,'Output filename: ',a)
        ELSE
         WRITE(Ng,1100)i,Insrs(Unopnd(i))(1:n1)
 1100    FORMAT(2x,i3,2x,'Input filename:  ',a,/,
     &                7x,'Output filename: NOT SPECIFIED')
        END IF
       END DO
      END IF
C-----------------------------------------------------------------------
      RETURN
      END
