      SUBROUTINE upbuf(Linno,Lin,Linln,buf,bufln,begbuf,endbuf,crntbf)
c-----------------------------------------------------------------------
c     upbuf.f, Release 1, Subroutine Version 1.1, Modified 18 Nov 2007.
c-----------------------------------------------------------------------
      IMPLICIT NONE
      INCLUDE 'lex.i'
      INCLUDE 'cchars.i'
      INCLUDE 'stdio.i'
c     -----------------------------------------------------------------
      CHARACTER Lin*(*),nxtchr
      INTEGER i,Linln,Linno
c     -----------------------------------------------------------------
      INTEGER nblank
      EXTERNAL nblank
c-----------------------------------------------------------------------
      CHARACTER buf(0:PBUFSZ-1)*(LINLEN)
      INTEGER bufln(0:PBUFSZ-1),begbuf,endbuf,crntbf
c     -----------------------------------------------------------------
      endbuf=mod(endbuf+1,PBUFSZ)
      IF(begbuf.eq.endbuf)begbuf=mod(begbuf+1,PBUFSZ)
      crntbf=mod(crntbf+1,PBUFSZ)
c-----------------------------------------------------------------------
c     Tack on an EOL
c-----------------------------------------------------------------------
      Linln=nblank(Lin)
      Linln=Linln+1
      IF(Linln.gt.LINLEN)THEN
       WRITE(STDERR,*)' ERROR: Input record longer than limit :',
     &                LINLEN
       CALL abend()
       RETURN  
      END IF
      Lin(Linln:Linln)=NEWLIN
c-----------------------------------------------------------------------
c     Filter out all unprintable characters
c-----------------------------------------------------------------------
      i=1
c     -----------------------------------------------------------------
      DO WHILE (.true.)
*        i=i+1
c     -----------------------------------------------------------------
       IF(i.lt.Linln)THEN
        nxtchr=Lin(i:i)
c     -----------------------------------------------------------------
c     Change by BCM to allow tab characters to be read in spec file
c     and not skipped over - May 2005
c     -----------------------------------------------------------------
        IF((nxtchr.lt.' '.or.nxtchr.gt.'~').and.
     &     (.not.(nxtchr.eq.TABCHR)))THEN
C          CALL inpter(PERROR,Pos,'Skipped over unprintable character her
C     &e')
         Lin(i:Linln-1)=Lin(i+1:Linln)
         Linln=Linln-1
c     -----------------------------------------------------------------
        ELSE
         i=i+1
        END IF
        GO TO 30
       END IF
c     -----------------------------------------------------------------
       GO TO 40
   30  CONTINUE
      END DO
c-----------------------------------------------------------------------
c     Store the next line in the buffer and return
c-----------------------------------------------------------------------
   40 buf(endbuf)=Lin
      bufln(endbuf)=Linln
c     -----------------------------------------------------------------
      RETURN
      END