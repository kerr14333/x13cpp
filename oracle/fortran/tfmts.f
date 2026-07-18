C     Last change:  BCM  30 Sep 1998    9:09 am
**==tfmts.f    processed by SPAG 4.03F  at 09:54 on  1 Mar 1994
      SUBROUTINE tfmts(Ny,Outdec,Maxy,Miny,Muladd,Lwidpr,Readok)
      IMPLICIT NONE
c-----------------------------------------------------------------------
C --- THIS SUBROUTINE GENERATES THE FORMATS FOR SUBROUTINE TABLES.
c     as well as column headers.
c-----------------------------------------------------------------------
      INCLUDE 'stdio.i'
      INCLUDE 'srslen.prm'
      INCLUDE 'tfmts.cmn'
      INCLUDE 'units.cmn'
      INCLUDE 'error.cmn'
c-----------------------------------------------------------------------
c     Include data dictionary of table formats
c-----------------------------------------------------------------------
      INCLUDE 'tfmts.prm'
c     ------------------------------------------------------------------
      LOGICAL T,F
      PARAMETER(T=.true.,F=.false.)
c-----------------------------------------------------------------------
      DOUBLE PRECISION Miny,Maxy
      LOGICAL Lwidpr,Readok
      INTEGER Ny,Outdec,Muladd,obswid,fac,ipos,ipos2,ifmt,npos,itmp
      CHARACTER blnk*22,cmonth*3,cqtr*3,fbase*110,fobs*5,fsum*5,stmp*3
      DIMENSION cmonth(12),cqtr(4)
c-----------------------------------------------------------------------
      LOGICAL dpeq
      EXTERNAL dpeq
c-----------------------------------------------------------------------
      DATA cmonth/'Jan','Feb','Mar','Apr','May','Jun','Jul','Aug','Sep',
     &     'Oct','Nov','Dec'/
      DATA cqtr/'1st','2nd','3rd','4th'/
c-----------------------------------------------------------------------
      INCLUDE 'tfmts.var'
c-----------------------------------------------------------------------
c --- initialize format variables to blanks.
c-----------------------------------------------------------------------
      CALL setchr(' ',110,Ifmt1)
      CALL setchr(' ',110,Ifmt2)
c-----------------------------------------------------------------------
c --- set formats for TABLE subroutine.  First, determine how wide the
c --- data should be
c-----------------------------------------------------------------------
      IF(Muladd.eq.1.and.abs(Miny).gt.Maxy)THEN
       obswid=int(log10(abs(Miny)))+Outdec+3
      ELSE
       IF(dpeq(Maxy,0D0))THEN
        obswid=Outdec+3
       ELSE
        obswid=int(log10(Maxy))+Outdec+3
       END IF
      END IF
      IF(Muladd.eq.0.or.Muladd.eq.2)THEN
       fac=Outdec+4
       IF(Outdec.eq.0)fac=fac+2
       IF(fac.gt.obswid)obswid=fac
      END IF
c-----------------------------------------------------------------------
c --- Set the number of observations per line and the width of the 
c --- obsertions to be printed out according to the observation width.
c-----------------------------------------------------------------------
      IF(obswid.le.8)THEN
       Tblcol=6
       Tblwid=8
      ELSE IF(obswid.le.12)THEN
       Tblcol=4
       Tblwid=12
      ELSE IF(obswid.le.15)THEN
       Tblcol=3
       Tblwid=15 
      ELSE IF(obswid.le.21)THEN
       Tblcol=2
       Tblwid=21
      ELSE
       CALL writln('ERROR: Data too large for '//PRGNAM//
     &             ' print format.',STDERR,Mt2,T)
       CALL writln(
     &   '       Try dividing the series by power of 10, or use the',
     &             STDERR,Mt2,F)
       CALL writln('       divpower argument found in the series and com
     &posite specs.',STDERR,Mt2,F)
       Readok=F
       RETURN
c       Tblcol=3
c       Tblwid=15
      END IF
      IF(Lwidpr)Tblcol=Tblcol*2
      IF(Ny.eq.4.and.Tblcol.gt.4)THEN
       Tblcol=4
       IF(Lwidpr)THEN
        Tblwid=15
       ELSE IF(Tblwid.le.12)THEN
        Tblwid=12
       ELSE IF (Tblwid.eq.15)THEN
        Tblcol=3
       ELSE
        Tblcol=2
       END IF
      END IF
c-----------------------------------------------------------------------
c     Construct the two formats
c-----------------------------------------------------------------------
      IF(Tblcol.eq.3)THEN
       Iptr=1
      ELSE IF(Tblcol.eq.4)THEN
       Iptr=5
      ELSE IF(Tblcol.eq.6)THEN
       Iptr=13
      ELSE IF(Tblcol.eq.8)THEN
       Iptr=17
      ELSE IF(Tblcol.eq.12)THEN
       Iptr=19
      ELSE IF(Tblcol.eq.2)THEN
       Iptr=21
      END IF
      IF(Ny.eq.4)Iptr=Iptr+2
      IF(Lwidpr.and.Tblcol.eq.4.and.Tblwid.le.15)Iptr=Iptr+4
      IF(Lwidpr.and.Tblcol.eq.6)Iptr=Iptr+2
c-----------------------------------------------------------------------
c     set up observation formats
c-----------------------------------------------------------------------
      IF(Tblwid.gt.9)then
       write(fobs,1010)Tblwid,Outdec
 1010  format('f',i2,'.',i1)
       ifmt=5
      ELSE
       write(fobs,1020)Tblwid,Outdec
 1020  format('f',i1,'.',i1)
       ifmt=4
      end if
      write(fsum,1010)Tblwid+2,Outdec
      fbase=' '
      CALL getstr(TFMDIC,tfmptr,PTFM,Iptr,fbase,ipos)
      IF(Lfatal)RETURN
      CALL cnvfmt(fbase,Ifmt1,fobs(1:ifmt),fsum,ipos,Nfmt1)
      fbase=' '
      CALL getstr(TFMDIC,tfmptr,PTFM,Iptr+1,fbase,ipos)
      IF(Lfatal)RETURN
      CALL cnvfmt(fbase,Ifmt2,fobs(1:ifmt),fsum,ipos,Nfmt2)
c-----------------------------------------------------------------------
c     Generate displacement indexes
c-----------------------------------------------------------------------
      Disp1=4
      IF(Iptr.eq.1.or.Iptr.eq.5.or.Iptr.eq.7.or.Iptr.ge.21)Disp1=3
      Disp2=1
      IF(Iptr.eq.9.or.Iptr.eq.11)Disp2=6
      Disp3=4
      IF(Iptr.eq.3)THEN
       Disp3=35
      ELSE IF(Iptr.eq.17)THEN
       Disp3=56
      ELSE IF(Iptr.eq.5.or.Iptr.eq.7)THEN
       Disp3=3
      ELSE IF(Iptr.eq.9.or.Iptr.eq.11)THEN
       Disp3=10
      END IF
c-----------------------------------------------------------------------
c     Generate format for summary statistics at the end of the printout
c-----------------------------------------------------------------------
      CALL tfmts3(Outdec,Muladd,Tblwid,Lwidpr,Ifmt3)
      IF(Lfatal)RETURN
c-----------------------------------------------------------------------
c     Generate vector for column headers.  First, construct format for
c     column headings.
c-----------------------------------------------------------------------
      if(Tblwid.gt.9)then
       write(fobs,1050)Tblwid
 1050  format('a',i2)
       ifmt=3
      else
       write(fobs,1060)Tblwid
 1060  format('a',i1)
       ifmt=2
      end if
      write(fsum,1050)Tblwid+2
      fbase=' '
      CALL getstr(TFMDIC,tfmptr,PTFM,Iptr,fbase,ipos)
      IF(Lfatal)RETURN
      CALL cnvfmt(fbase,Fmtcol,fobs(1:ifmt),fsum(1:3),ipos,npos)
      Fmtcol(1:npos)=Ifmt2(1:6)//Fmtcol(7:npos)
c-----------------------------------------------------------------------
c     Set hdr array to be the name of the month or quarter
c-----------------------------------------------------------------------
      CALL setchr(' ',22,blnk)
      Colhdr(1)=blnk
      DO ipos=1,Ny
       ipos2=ipos+1
       Colhdr(ipos2)=blnk
       IF(Ny.eq.12)THEN
        Colhdr(ipos2)((Tblwid-3):(Tblwid-1))=cmonth(ipos)
       ELSE IF(Ny.eq.4)THEN
        Colhdr(ipos2)((Tblwid-3):(Tblwid-1))=cqtr(ipos)
       ELSE
        itmp=1
        CALL itoc(ipos,stmp,itmp)
        Colhdr(ipos2)((Tblwid-itmp):(Tblwid-1))=stmp(1:(itmp-1))
       END IF
      END DO
c-----------------------------------------------------------------------
      RETURN
      END
