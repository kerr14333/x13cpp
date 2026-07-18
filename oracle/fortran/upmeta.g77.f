      SUBROUTINE upmeta(Insrs,Outsrs,Datsrs,Imeta,Mtafil,Ldata,Dtafil,
     &                  Mtalin,Nmeta,Nfil,blnk,quot)
      IMPLICIT NONE
C-----------------------------------------------------------------------
      INCLUDE 'stdio.i'
      INCLUDE 'notset.prm'
C-----------------------------------------------------------------------
      CHARACTER Insrs*(PFILCR),Outsrs*(PFILCR),Datsrs*(PFILCR),blnk*1,
     &          Mtalin*(*),Dtafil*(PFILCR),Mtafil*(PFILCR),quot*1
      LOGICAL Ldata
      INTEGER i,i2,Imeta,j,n,Nmeta,Nfil,ichr,nchr,n1,n2
      DIMENSION Insrs(PSRS),Outsrs(PSRS),Datsrs(PSRS)
C-----------------------------------------------------------------------
      INTEGER nblank,lstpth
      EXTERNAL nblank,lstpth
c-----------------------------------------------------------------------
      Outsrs(Imeta)=blnk
C-----------------------------------------------------------------------
c     If this is a blank line (line of length zero), decrement the 
c     series counter and process the next line. 
C-----------------------------------------------------------------------
      IF(Nmeta.eq.0)THEN
       Imeta=Imeta-1
      ELSE
C----------------------------------------------------------------------
c     If the first character of the line is a quotation mark,
c     Find the next quotation mark.
c     November 2005 - BCM
C-----------------------------------------------------------------------
       IF(Mtalin(1:1).eq.quot)THEN
        i=2
        DO WHILE (Mtalin(i:i).ne.quot.and.i.le.Nmeta)
         i=i+1
        END DO
        IF (i.eq.Nmeta.and.Mtalin(Nmeta:Nmeta).ne.quot)THEN
         IF(Ldata)THEN
          WRITE(STDERR,1021)'data',Mtafil(1:Nfil)
         ELSE
          WRITE(STDERR,1021)'input',Mtafil(1:Nfil)
         END IF
         CALL abend
         RETURN
        END IF
C-----------------------------------------------------------------------
c     Set the length of the first string.  
C-----------------------------------------------------------------------
        n=i
        n1=2
        n2=n-1
       ELSE
C-----------------------------------------------------------------------
c     Find the first blank or not set character
C-----------------------------------------------------------------------
        i=1
        DO WHILE (Mtalin(i:i).ne.blnk.and.i.le.Nmeta)
         i=i+1
        END DO
C-----------------------------------------------------------------------
c     If the first character of a line is a blank character, print an 
c     error message 
C-----------------------------------------------------------------------
        IF(i.eq.1)THEN
         IF(Ldata)THEN
          WRITE(STDERR,1020)' data',Mtafil(1:Nfil)
         ELSE
          WRITE(STDERR,1020)'n input',Mtafil(1:Nfil)
         END IF
         CALL abend
         RETURN
        END IF
C-----------------------------------------------------------------------
c     Set the length of the first string.  
C-----------------------------------------------------------------------
        n=i-1
        n1=1
        n2=n
       END IF
C-----------------------------------------------------------------------
c     If this is an input metafile, store the series name in the 
c     variable series.  Else, store as an element of Dtasrs
C-----------------------------------------------------------------------
       IF(Ldata)THEN
        Datsrs(Imeta)=Mtalin(n1:n2)
        Insrs(Imeta)=Infile
       ELSE
        Insrs(Imeta)=Mtalin(n1:n2)
       END IF
C-----------------------------------------------------------------------
c     Is the end of the first string the end of the line?  If so,
c     set output names.
C-----------------------------------------------------------------------
       IF(Nmeta.eq.n)THEN
c     ------------------------------------------------------------------
c     If data metafile is used, get the path and filename from the 
c     datafile to use as the output file name.
c     ------------------------------------------------------------------
        IF(Ldata)THEN
         ichr=lstpth(Mtalin,n)+1
         DO i2=n2,ichr,-1
          IF(Mtalin(i2:i2).eq.'.')THEN
           nchr=i2-1
           GO TO 30
          END IF
         END DO
         nchr=n2
   30    Outsrs(Imeta)=Mtalin(n1:nchr)
        ELSE
c     ------------------------------------------------------------------
c     If an input metafile is used, set the output file to be the same 
c     as the spec file.
c     ------------------------------------------------------------------
         Outsrs(Imeta)=Mtalin(n1:n2)
        END IF
C-----------------------------------------------------------------------
c     If not, find the position of the next non-blank character
C-----------------------------------------------------------------------
       ELSE
        IF(Mtalin(i:i).eq.quot)i=i+1
        DO WHILE (Mtalin(i:i).eq.blnk)
         i=i+1
        END DO
C-----------------------------------------------------------------------
C     Check to see if there are any more blanks in the line
C-----------------------------------------------------------------------
        j=i
        IF(Mtalin(j:j).eq.quot)THEN
         j=j+1
         DO WHILE (Mtalin(j:j).ne.quot.and.j.le.Nmeta)
          j=j+1
         END DO
         IF (i.eq.Nmeta.and.Mtalin(Nmeta:Nmeta).ne.quot)THEN
          IF(Ldata)THEN
           WRITE(STDERR,1021)'data',Mtafil(1:Nfil)
          ELSE
           WRITE(STDERR,1021)'input',Mtafil(1:Nfil)
          END IF
          CALL abend
          RETURN
         END IF
C-----------------------------------------------------------------------
c     Store the output file name in the array Outsrs
C-----------------------------------------------------------------------
         Outsrs(Imeta)=Mtalin((i+1):(j-1))
        ELSE
C-----------------------------------------------------------------------
         DO WHILE (Mtalin(j:j).ne.blnk.and.j.le.Nmeta)
          j=j+1
         END DO
C-----------------------------------------------------------------------
c     Store the output file name in the array Outsrs
C-----------------------------------------------------------------------
         Outsrs(Imeta)=Mtalin(i:(j-1))
        END IF
       END IF
      END IF
C-----------------------------------------------------------------------
 1020 FORMAT(/,' ERROR: The first entry in each line of a',a,
     &         ' metafile must be left ',
     &       /,'        justified.  Correct the metafile and rerun ',a,
     &         '.')
 1021 FORMAT(/,' ERROR: Closing quotation mark not found in this ',a,
     &         ' metafile.',
     &       /,'        Correct the metafile and rerun ',a,'.')
C-----------------------------------------------------------------------
      RETURN
      END