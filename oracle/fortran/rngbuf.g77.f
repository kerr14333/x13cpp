C     Last change:  BCM  13 May 2005    1:42 pm
      LOGICAL FUNCTION rngbuf(Cmd,Linno,Lin,Linln)
c-----------------------------------------------------------------------
c     rngbuf.f, Release 1, Subroutine Version 1.2, Modified 03 Feb 1995.
c-----------------------------------------------------------------------
      IMPLICIT NONE
      INCLUDE 'lex.i'
      INCLUDE 'cchars.i'
      INCLUDE 'stdio.i'
      INCLUDE 'error.cmn'
c     -----------------------------------------------------------------
      LOGICAL PFAIL,PSCCD
      PARAMETER(PFAIL=.false.,PSCCD=.true.)
c     -----------------------------------------------------------------
      CHARACTER Lin*(*),nxtchr
      INTEGER Cmd,i,Linln,Linno
c     -----------------------------------------------------------------
      INTEGER nblank
      EXTERNAL nblank
c-----------------------------------------------------------------------
      LOGICAL psteof
      CHARACTER buf(0:PBUFSZ-1)*(LINLEN)
      INTEGER bufln(0:PBUFSZ-1),begbuf,endbuf,crntbf,crntln
      SAVE buf,bufln,begbuf,endbuf,crntbf,crntln,psteof
c-----------------------------------------------------------------------
c     Which command
c-----------------------------------------------------------------------
      rngbuf=PSCCD
c     -----------------------------------------------------------------
      GO TO(10,20,50,60),Cmd
      WRITE(STDERR,*)'System error:  illegal buffer request,',Cmd
      CALL abend()
      RETURN
c-----------------------------------------------------------------------
c     Initialize the Buffer
c-----------------------------------------------------------------------
   10 crntln=0
      begbuf=PBUFSZ-1
      endbuf=PBUFSZ-1
      crntbf=endbuf
      bufln(0)=0
      psteof=.false.
      Lexok=.true.
      GO TO 80
c-----------------------------------------------------------------------
c     Read the next line and fail if EOF
c-----------------------------------------------------------------------
   20 IF(psteof)GO TO 70
c-----------------------------------------------------------------------
c     Read in a new line if necessary
c-----------------------------------------------------------------------
      IF(crntbf.eq.endbuf)THEN
       READ(Inputx,1010,END=70)Lin
 1010  FORMAT(a)
       CALL upbuf(Linno,Lin,Linln,buf,bufln,begbuf,endbuf,crntbf)
       IF(Lfatal)RETURN
c     -----------------------------------------------------------------
      ELSE
       crntbf=mod(crntbf+1,PBUFSZ)
       Lin=buf(crntbf)
       Linln=bufln(crntbf)
      END IF
c     -----------------------------------------------------------------
      crntln=crntln+1
      Linno=crntln
      GO TO 80
c-----------------------------------------------------------------------
c     Push back the last line on to the stack
c-----------------------------------------------------------------------
   50 IF(crntbf.eq.begbuf)THEN
       rngbuf=PFAIL
       Linln=0
c-----------------------------------------------------------------------
c     If we are at the EOF we don't need to back up, just return the
c last line in the file.
c-----------------------------------------------------------------------
      ELSE
       IF(.not.psteof)THEN
        crntbf=mod(crntbf+PBUFSZ-1,PBUFSZ)
        crntln=crntln-1
       END IF
c     -----------------------------------------------------------------
       Lin=buf(crntbf)
       Linln=bufln(crntbf)
       Linno=crntln
       psteof=.false.
      END IF
      GO TO 80
c-----------------------------------------------------------------------
c     Get a line Lineno and fail if the line isn't in the buffer
c-----------------------------------------------------------------------
   60 IF(Linno.lt.crntln-mod(crntbf+PBUFSZ-begbuf,PBUFSZ).or.
     &   Linno.gt.crntln)THEN
       rngbuf=PFAIL
       Linln=0
c     -----------------------------------------------------------------
      ELSE
       i=mod(crntbf+Linno-crntln+PBUFSZ,PBUFSZ)
       Lin=buf(i)
       Linln=bufln(i)
      END IF
c     END CASE STATEMENT
      GO TO 80
c-----------------------------------------------------------------------
c     Reached the EOF
c-----------------------------------------------------------------------
   70 Linln=nblank(Lin)
      IF(Linln.gt.0)THEN
       CALL upbuf(Linno,Lin,Linln,buf,bufln,begbuf,endbuf,crntbf)
       IF(Lfatal)RETURN
      END IF
      Lin(1:1)=CHREOF
      Linln=1
      psteof=.true.
      rngbuf=PFAIL
c     -----------------------------------------------------------------
   80 RETURN
      END
