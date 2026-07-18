C     Last change:  BCM   1 Jun 1998    4:08 pm
**==genfor.f    processed by SPAG 4.03F  at 11:36 on 10 Jun 1994
      SUBROUTINE genfor(Ok,Lchkin,Isrs)
      IMPLICIT NONE
c     ------------------------------------------------------------------
C --- THIS SUBROUTINE PRINTS THE HEADINGS FOR THE VARIOUS FILES AND
C --- INITIALIZES VALUES.
C --- THE UNIT MT IS THE CONTROL CARD INPUT FILE
C ---          MT1 IS THE MAIN PRINTOUT
C ---          MT2 IS THE LOG
C ---          NG IS THE FILE CONTAINING ALL THE Q STATISTICS
C ---          NFORM CONTAINS ALL THE F TABLES FOR THE RUN
C ---          NR IS THE TAPE INPUT FILE AND IS DEFINED IN THE ROUTINE
C ---                  INPUT
c ---  Note - not all of these variables are set in this routine (BCM)
c     ------------------------------------------------------------------
      INCLUDE 'stdio.i'
      INCLUDE 'agr.cmn'
      INCLUDE 'units.cmn'
      INCLUDE 'title.cmn'
      INCLUDE 'filetb.cmn'
c     ------------------------------------------------------------------
      LOGICAL F
      PARAMETER(F=.false.)
c     ------------------------------------------------------------------
      CHARACTER ext*4,fil*(PFILCR)
      LOGICAL Ok,lok,Lchkin
      INTEGER Isrs,nfil
c     ------------------------------------------------------------------
      INTEGER nblank
      EXTERNAL nblank
c     ------------------------------------------------------------------
c     For first series, initialize values
c     ------------------------------------------------------------------
      IF(Isrs.eq.1)THEN
       Newpg=char(12)
       Iagr=0
       Nform=0
      END IF
c     ------------------------------------------------------------------
      Mt1=0
      Mt2=0
c     ------------------------------------------------------------------
c     Check filenames for improper file extensions
c     ------------------------------------------------------------------
      Nfilcr=nblank(Cursrs)
      nfil=nblank(Infile)
      IF(nfil.gt.3)THEN
       ext=Infile((nfil-3):nfil)
       IF((ext(1:2).eq.'.s'.or.ext(1:2).eq.'.S').and.
     &   (ext(3:3).eq.'p'.or.ext(3:3).eq.'P').and.
     &   (ext(4:4).eq.'c'.or.ext(4:4).eq.'C'))THEN
        WRITE(STDERR,1010)'input spec',ext
 1010   FORMAT(' ERROR: Enter ',a,' filename without "',a,
     &         '" file extension.')
        Ok=F
       END IF
      END IF
      IF(Nfilcr.gt.3)THEN
       ext=Cursrs((Nfilcr-3):Nfilcr)
       IF((ext(1:2).eq.'.o'.or.ext(1:2).eq.'.O').and.
     &   (ext(3:3).eq.'u'.or.ext(3:3).eq.'U').and.
     &   (ext(4:4).eq.'t'.or.ext(4:4).eq.'T'))THEN
        WRITE(STDERR,1010)'output',ext
        Ok=F
       END IF
      END IF
      IF(nfil.eq.0)THEN
       WRITE(STDERR,1011)
 1011  FORMAT('  No filename specified for input specification file.')
       Ok=F
      ELSE IF(Nfilcr.eq.0)THEN
       WRITE(STDERR,1012)
 1012  FORMAT('  No output filename specified.')
       Ok=F
      END IF
      IF(.not.Ok)THEN
       CALL abend
       RETURN
      END IF
c     ------------------------------------------------------------------
c     Try to open output file
c     ------------------------------------------------------------------
      fil=Cursrs(1:Nfilcr)//'.out'
      nfil=Nfilcr+4
      INQUIRE(FILE=fil(1:nfil),EXIST=Lexout)
      CALL fopen(fil(1:nfil),'program output file','UNKNOWN',Mt1,lok)
      Ok=Ok.and.lok
c     ------------------------------------------------------------------
c     Try to open spec file
c     ------------------------------------------------------------------
      IF(Ok)THEN
       nfil=nblank(Infile)
       Infile=Infile(1:nfil)//'.spc'
       nfil=nfil+4
       CALL fopen(Infile(1:nfil),'input spec file','OLD',Mt,lok)
       Ok=Ok.and.lok
      END IF
c     ------------------------------------------------------------------
c     Try to open error file
c     ------------------------------------------------------------------
      IF(Ok)THEN
       fil=Cursrs(1:Nfilcr)//'.err'
       nfil=Nfilcr+4
       INQUIRE(FILE=fil(1:nfil),EXIST=Lexerr)
       CALL fopen(fil(1:nfil),'program error file','UNKNOWN',Mt2,lok)
       Ok=Ok.and.lok
      END IF
c     ------------------------------------------------------------------
c     Print out summary of files opened by this routine if all files
c     have been opened
c     ------------------------------------------------------------------
      IF(Ok)THEN
       nfil=nblank(Infile)
       WRITE(STDOUT,1030)Infile(1:nfil),Cursrs(1:Nfilcr)//'.out',
     &                   Cursrs(1:Nfilcr)//'.err'
 1030  FORMAT(/,'  Reading input spec file from ',a,/,
     &        '  Storing any program output into ',a,/,
     &        '  Storing any program error messages into ',a)
c     ------------------------------------------------------------------
       WRITE(Mt2,1020)PRGNAM,Infile(1:nfil)
 1020  FORMAT(5x,'Error messages generated from processing the ',a,
     &           ' spec file',/,5x,A,':',//)
c     ------------------------------------------------------------------
c     If all files have not been opened, close all files and return
c     ------------------------------------------------------------------
      ELSE
       CALL fclose(-1)
       RETURN
      END IF
c     ------------------------------------------------------------------
      IF((.not.Lchkin).and.Lpage)Kpage=1
c     ------------------------------------------------------------------
      RETURN
      END
