C     Last change:  BCM   1 Feb 98   11:11 pm
      SUBROUTINE inpter(Errtyp,Ptr,Errmsg)
      IMPLICIT NONE
c-----------------------------------------------------------------------
c     inpter.f, Release 1, Subroutine Version 1.3, Modified 20 Oct 1994.
c-----------------------------------------------------------------------
c Errtyp=1 input error, 2 input warning, 3 execution error, 4 execution
c warning.
c-----------------------------------------------------------------------
      INCLUDE 'lex.i'
      INCLUDE 'stdio.i'
      INCLUDE 'units.cmn'
      INCLUDE 'title.cmn'
c-----------------------------------------------------------------------
      CHARACTER Errmsg*(*),errstr*7,blnkln*(LINLEN),prglin*(LINLEN)
      LOGICAL lprtln,rngbuf
      INTEGER displc,Errtyp,nerrcr,Ptr(2),prgln,llim,llim2
      EXTERNAL rngbuf
      SAVE blnkln
      DATA blnkln/
     &'                                                                 
     &                                                                  
     &  '/
c-----------------------------------------------------------------------
c     For input errors and warnings print out the input line and carrot.
c-----------------------------------------------------------------------
      lprtln=.false.
      llim=70
      IF(Lwdprt)llim=121
c     ------------------------------------------------------------------
      IF(Errtyp.eq.PERROR.or.Errtyp.eq.PWARN)THEN
       lprtln=rngbuf(4,Ptr(PLINE),prglin,prgln)
       IF(lprtln)THEN
        IF(prgln-1.gt.llim)THEN
         WRITE(STDERR,1010)Ptr(PLINE),prglin(1:min((llim+10),prgln-1))
         WRITE(Mt2,1010)Ptr(PLINE),prglin(1:min((llim+10),prgln-1))
 1010    FORMAT(/,' Line',i5,':  ',/,' ',a)
         displc=0
        ELSE
         WRITE(STDERR,1020)Ptr(PLINE),prglin(1:prgln-1)
         WRITE(Mt2,1020)Ptr(PLINE),prglin(1:prgln-1)
 1020    FORMAT(/,' Line',i5,':  ',a)
         displc=12
        END IF
c-----------------------------------------------------------------------
c     Put the carrot on the input error
c-----------------------------------------------------------------------
        WRITE(STDERR,1030)blnkln(1:Ptr(PCHAR)+displc)//'^'
        WRITE(Mt2,1030)blnkln(1:Ptr(PCHAR)+displc)//'^'
 1030   FORMAT(a)
       END IF
      END IF
c-----------------------------------------------------------------------
c     Print out the error message
c-----------------------------------------------------------------------
      IF(mod(Errtyp,2).eq.0)THEN
       nerrcr=7
       errstr(1:nerrcr)='WARNING'
      ELSE
       nerrcr=5
       errstr(1:nerrcr)='ERROR'
      END IF
c     ------------------------------------------------------------------
      IF(.not.lprtln.or.(Errtyp.ne.PERROR.and.Errtyp.ne.PWARN))THEN
       WRITE(STDERR,'()')
       WRITE(Mt2,'()')
      END IF
c     ------------------------------------------------------------------
      IF(len(Errmsg).le.llim)THEN
       WRITE(STDERR,1040)errstr(1:nerrcr),Errmsg
       WRITE(Mt2,1040)errstr(1:nerrcr),Errmsg
 1040  FORMAT(' ',a,':  ',a)
      ELSE
       llim2=llim
   10  IF(Errmsg(llim2:llim2).eq.' ')GO TO 20
       llim2=llim2-1
       GO TO 10
   20  WRITE(STDERR,1050)errstr(1:nerrcr),Errmsg(1:llim2),
     &                   blnkln(1:nerrcr),Errmsg((llim2+1):)
       WRITE(Mt2,1050)errstr(1:nerrcr),Errmsg(1:llim2),
     &                blnkln(1:nerrcr),Errmsg((llim2+1):)
 1050  FORMAT(' ',a,':  ',a,/,' ',a,'   ',a)
      END IF
c     ------------------------------------------------------------------
      IF(.not.lprtln.and.(Errtyp.eq.PERROR.or.Errtyp.eq.PWARN))THEN
       WRITE(STDERR,1060)blnkln(1:nerrcr+3),Ptr
       WRITE(Mt2,1060)blnkln(1:nerrcr+3),Ptr
 1060  FORMAT(a,' Problem was discovered on line',i5,', column ',i4,'.')
c      ELSE
c       WRITE(STDERR,'()')
c       WRITE(Mt2,'()')
      END IF
c     -----------------------------------------------------------------
      RETURN
      END
