C     Last change: Nov, 2021, add a logical variable to test if this is
C     the first .spc which has composite spec, program only processes
C     to this file. The .spcs after this file will be ignored.
C     previous change:  BCM  15 Oct 1998   12:21 pm
**==aa0001.f    processed by SPAG 4.03F  at 10:52 on 28 Sep 1994
      BLOCK DATA INX12
      IMPLICIT NONE
C-----------------------------------------------------------------------
      INCLUDE 'srslen.prm'
      INCLUDE 'ssap.prm'
      INCLUDE 'chrt.cmn'
      INCLUDE 'ssap.cmn'
C-----------------------------------------------------------------------
      INTEGER j,ii
C-----------------------------------------------------------------------
      DATA Ialpha/'j','f','m','a','m','j','j','a','s','o','n','d'/
      DATA Ialphq/'1','2','3','4'/
      DATA I1,I4,I7/'*','I','.'/
      DATA Imid/2,6,10,14,18,22,26,30,34,38,42,46,50,54/
      DATA F1/'(1x,i2,a1,i4,2x, (f9.2,2x),3x,f9.2,2x,a10)'/
      DATA F2/'(11x, (1x,i2,a1,i4,1x,a1,1x),4x,a7,3x,a8)'/
      DATA F3/'(1x,i2,a1,i4,2x, (e10.4,1x),3x,f9.2,2x,a10)'/
      DATA(Cut(1,ii),ii=1,4)/3D0,4D0,5D0,6D0/
      DATA(Cut(2,ii),ii=1,4)/2D0,3D0,4D0,5D0/
      DATA(Cut(3,ii),ii=1,4)/3D0,4D0,5D0,6D0/
      DATA(Cut(4,ii),ii=1,4)/3D0,5D0,7D0,10D0/
      DATA(Cut(5,ii),ii=1,4)/3D0,4D0,5D0,6D0/
      DATA(Ch(j),j=1,NEST)/'%','+','#','$','@'/
C-----------------------------------------------------------------------
      END
**==x12a.f    processed by SPAG 4.03F  at 10:53 on 28 Sep 1994
      PROGRAM x12a
      IMPLICIT NONE
C-----------------------------------------------------------------------
      INCLUDE 'stdio.i'
      INCLUDE 'lex.i'
      INCLUDE 'cchars.i'
      INCLUDE 'build.prm'
      INCLUDE 'notset.prm'
      INCLUDE 'seatop.cmn'
      INCLUDE 'units.cmn'
      INCLUDE 'hiddn.cmn'
      INCLUDE 'title.cmn'
      INCLUDE 'error.cmn'
      INCLUDE 'nsums.i'
C-----------------------------------------------------------------------
      LOGICAL lmeta,lchkin,rok,lcomp,ldata,lgraf,lexgrf,gmtok,x11agr,
     &        fok,l1stcomp
      CHARACTER insrs*(PFILCR),outsrs*(PFILCR),mtafil*(PFILCR),
     &          datsrs*(PFILCR),logfil*(PFILCR),dtafil*(PFILCR),
     &          grfdir*(PFILCR),xb*(PFILCR),tfmt*(10),dattim*(24)
      INTEGER i,failed,nfail,unopnd,nopen,nlgfil,n1,n2,xfail,
     &        nmtfil,n4,i1,i2,ilghdr
      DIMENSION outsrs(PSRS),insrs(PSRS),datsrs(PSRS),failed(PSRS),
     &          unopnd(PSRS)
C-----------------------------------------------------------------------
      CHARACTER*24 cvdttm
      INTEGER nblank,lstpth
      EXTERNAL nblank,lstpth,cvdttm
C-----------------------------------------------------------------------
      LOGICAL F,T
      PARAMETER(F=.false.,T=.true.)
C-----------------------------------------------------------------------
c     Initialize variables
C-----------------------------------------------------------------------
      Mt2=0
      Ng=8
      STDERR=6
      nfail=0
      xfail=0
      nopen=0
      Lfatal=F
      x11agr=T
      l1stcomp=F
      CALL setchr(' ',PFILCR,Infile)
      CALL setchr(' ',PFILCR,Cursrs)
      CALL setchr(' ',PFILCR,Curgrf)
      CALL setchr(' ',PFILCR,grfdir)
      CALL setchr(' ',PFILCR,dtafil)
      CALL setchr(' ',PFILCR,xb)
      TABCHR=CHAR(9)
      Ierhdr=NOTSET
      Crvend=CNOTST
      CALL fdate(dattim)
      dattim=cvdttm(dattim)
cunix
      CHREOF=char(4)
      NEWLIN=char(10)
C     ------------------------------------------------------------------
cdos
cdos      CHREOF=char(26)
cdos      NEWLIN=char(13)
C-----------------------------------------------------------------------
C     Print out introductory header giving version number of program.
C-----------------------------------------------------------------------
       WRITE(STDOUT,1000)PRGNAM,VERNUM,BUILD,dattim
 1000  FORMAT(/,1x,a,' Seasonal Adjustment Program',/,
     &        ' Version Number ',a,' Build ',a,/,' Execution began ',a)
C-----------------------------------------------------------------------
c     Get options specified on the command line.
C-----------------------------------------------------------------------
      CALL getxop(lmeta,lchkin,lcomp,Lsumm,Lmdsum,Lnoprt,Lwdprt,Lpage,
     &            ldata,dtafil,lgraf,grfdir,Lcmpaq,Ltimer)
      IF(Lfatal)STOP
C-----------------------------------------------------------------------
      IF(Lwdprt)THEN
       Ttlfmt='(A1,8X,A,T92,''PAGE'',I4,'', SERIES '',A)'
      ELSE
       Ttlfmt='(A1,8X,A,12X,''PAGE'',I4,'', SERIES '',A)'
      END IF
C-----------------------------------------------------------------------
c     If input is from metafile, get list of series names
C-----------------------------------------------------------------------
      IF(lmeta)THEN
       CALL gtmtfl(insrs,outsrs,datsrs,mtafil,ldata,dtafil)
       IF(Lfatal)STOP
       nmtfil=nblank(mtafil)
       logfil=mtafil(1:(nmtfil-4))
      ELSE
C-----------------------------------------------------------------------
c     Else, set up variables for using a single file.
C-----------------------------------------------------------------------
       Imeta=1
       insrs(1)=Infile
       outsrs(1)=Cursrs
       logfil=outsrs(1) 
       mtafil=' '
       nmtfil=1
      END IF
C-----------------------------------------------------------------------
c     initialize variables for Lmdsum
C-----------------------------------------------------------------------
      nSeatsSer=0
      noTratadas=0
      call inicSumS()
C-----------------------------------------------------------------------
c     open log file for all X-13A-S runs.  First, get path information
C-----------------------------------------------------------------------
      nlgfil=nblank(logfil)
      logfil(nlgfil+1:nlgfil+4)='.log'
      nlgfil=nlgfil+4
      OPEN(Ng,FILE=logfil(1:nlgfil),STATUS='UNKNOWN',ERR=20)
      WRITE(Ng,1010)
 1010 FORMAT('1')
      IF(.not.lchkin)THEN
       ilghdr=67+nblank(VERNUM)+nblank(BUILD)
       IF(Lwdprt)THEN
        i1=(LINLEN-ilghdr)/2
       ELSE
        i1=(80-ilghdr)/2
       END IF
       WRITE(Ng,1020)xb(1:i1),PRGNAM,VERNUM,BUILD,dattim
 1020  FORMAT(A,'Log for ',a,' program (Version ',a,' Build ',a,') ',a)
       i2=ilghdr/2
       WRITE(tfmt,1021)i2
 1021  FORMAT('(a,',i2,'a2,a)')
       WRITE(Ng,tfmt)xb(1:i1),('*-',i=1,i2),'*'
       WRITE(Ng,1022)
 1022  FORMAT(///,
     &       ' Type of  Series',6x,'Additional',19x,'Series title',/,
     &       ' Adjust.  Ident.',6x,'Identifiers',/)
      END IF
C-----------------------------------------------------------------------
c     Process all the series.
C-----------------------------------------------------------------------
      DO i=1,Imeta
       rok=T
       dtafil=' '
       IF (l1stcomp) THEN
        WRITE(STDERR,1090)
        WRITE(Ng,1090)
        Imeta = i-1
        exit
       END IF
 1090 FORMAT(/,
     &  ' WARNING: X-13 will not process any series in a metafile ',/,
     &  '          following a spec for a composite adjustment.',/)
       IF(lmeta)THEN
        Infile=insrs(i)
        Cursrs=outsrs(i)
        IF(ldata)dtafil=datsrs(i)
       END IF
       n1=nblank(Infile)
       n2=nblank(Cursrs)
C-----------------------------------------------------------------------
c      Set up graphics variables
C-----------------------------------------------------------------------
       gmtok=T
       fok=T
       IF(lgraf)THEN
        Ngrfcr=nblank(grfdir)
        Curgrf(1:Ngrfcr)=grfdir(1:Ngrfcr)
        n4=lstpth(grfdir,Ngrfcr)
        IF(n4.lt.Ngrfcr)THEN
         Ngrfcr=Ngrfcr+1
cdos  backslash for directory
cdos         Curgrf(Ngrfcr:Ngrfcr)='\'
cunix forward slash for directory
         Curgrf(Ngrfcr:Ngrfcr)='/'
        END IF
        n4=lstpth(Cursrs,n2)
        Curgrf((Ngrfcr+1):(Ngrfcr+(n2-n4)))=Cursrs((n4+1):n2)
        Ngrfcr=Ngrfcr+n2-n4
        INQUIRE(FILE=Curgrf(1:Ngrfcr)//'.gmt',EXIST=lexgrf)
        CALL fopen(Curgrf(1:Ngrfcr)//'.gmt','graphical meta file',
     &             'UNKNOWN',Grfout,gmtok)
        IF(.not.gmtok)CALL abend
       END IF
C-----------------------------------------------------------------------
c       write(*,*) 'enter profiler'
c       call profiler(0,'Profiler.txt')
       IF(gmtok)THEN
        CALL x12run(i,unopnd,nopen,lchkin,lcomp,rok,fok,n1,nfail,ldata,
     &              dtafil,mtafil,nmtfil,dattim,x11agr,lgraf,lexgrf,
     &              l1stcomp)
       ELSE
        fok=F
       END IF
C-----------------------------------------------------------------------
c     print error message if there was an input error.
C-----------------------------------------------------------------------
       IF(Lfatal)THEN
        IF(rok)THEN
         WRITE(STDOUT,*)' Program error(s) halt execution for ',
     &                  Infile(1:n1),'.spc'
         xfail=xfail+1
        END IF
        IF(gmtok.and.fok)
     &     WRITE(STDOUT,1025)' Check error file '//Cursrs(1:n2)//'.err'
        nfail=nfail+1
        failed(nfail)=i
C-----------------------------------------------------------------------
       ELSE IF(.not.lchkin)THEN
        CALL fdate(dattim)
        dattim=cvdttm(dattim)
        WRITE(STDOUT,1025)' Execution complete for '//Infile(1:n1)//
     &                    '.spc at '//dattim
       END IF
C-----------------------------------------------------------------------
c     Close all files
C-----------------------------------------------------------------------
       CALL fclose(-1)
      END DO
C-----------------------------------------------------------------------
      IF((.not.Lquiet).and.nfail.gt.xfail)WRITE(STDERR,1030)
 1030 FORMAT(/,
     &  ' NOTE:  Correct input errors in the order they are detected',/,
     &  '        since the first one or two may be responsible for',/,
     &  '        the others (especially if there are errors in the',/,
     &  '        SERIES or COMPOSITE spec).',/)
C-----------------------------------------------------------------------
      IF(Imeta.gt.1)THEN
       IF(nopen.gt.0.or.nfail.gt.0)THEN
         CALL prtlog(Ng,insrs,outsrs,nopen,unopnd,nfail,failed,
     &               mtafil(1:nmtfil),logfil(1:nlgfil))
       END IF
       if (Lmdsum.and.nSeatsSer.gt.25) then
*          write(*,*)'  Lmdsum=T  nSeatsSer = ',nSeatsSer
          call writeSumS(mtafil,nmtfil-4,nSeatsSer,noTratadas,wSposBphi,
     $            wSstochTD,wSstatseas,wSrmod,wSxl)
*       else
*          write(*,*)'  Lmdsum=F  nSeatsSer = ',nSeatsSer
       end if
      END IF
C-----------------------------------------------------------------------
      IF(lgraf.and.Lsumm.gt.0)THEN
       WRITE(Ng,1040)
 1040  FORMAT(/,'  NOTE: The diagnostic files produced by the -s ',
     &        'option are stored in the',/,
     &       '        directory specified by the graphics (-g) option.')
      END IF
c     CALL fclose(Ng)
C-----------------------------------------------------------------------
      STOP
C-----------------------------------------------------------------------
   20 WRITE(STDERR,1025)' Unable to open '//logfil(1:nlgfil)
      CALL abend
 1025 FORMAT(/,a)
      STOP
      END
