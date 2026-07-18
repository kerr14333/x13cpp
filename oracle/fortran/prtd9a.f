C     Last change:  BCM  26 Apr 1998    2:48 pm
      SUBROUTINE prtd9a(Lprt)
      IMPLICIT NONE
c-----------------------------------------------------------------------
c     Print out the D9A (Moving Seasonality Ratio) table 
c-----------------------------------------------------------------------
      INCLUDE 'srslen.prm'
      INCLUDE 'tfmts.prm'
      INCLUDE 'tfmts.cmn'
      INCLUDE 'units.cmn'
      INCLUDE 'hiddn.cmn'
      INCLUDE 'error.cmn'
      INCLUDE 'x11opt.cmn'
c-----------------------------------------------------------------------
      LOGICAL F
      PARAMETER(F=.false.)
c-----------------------------------------------------------------------
      CHARACTER tfmt*(110),fobs*(5),fsum*(5),fbase*(110)
      DOUBLE PRECISION tmp
      LOGICAL Lprt
      INTEGER l,i,nline,n1,n2,n,ifmt,ipos,n3,npos
      DIMENSION tmp(PSP)
c-----------------------------------------------------------------------
      INCLUDE 'tfmts.var'
c-----------------------------------------------------------------------
c     Return if printing is turned off for this run.
c-----------------------------------------------------------------------
      IF(Lhiddn)RETURN
c-----------------------------------------------------------------------
c     If summary diagnostics are stored, store Ibar, Sbar and I/S
c-----------------------------------------------------------------------
       IF(Lsumm.gt.0)THEN
        DO i=1,Ny
         WRITE(Nform,1000)i,Rati(i),Rati(i+Ny),Rati(i+2*Ny)
        END DO
 1000   FORMAT('d9a.',i2.2,':',3(1x,E17.10))
        IF(.not.Lprt)RETURN
       END IF
c-----------------------------------------------------------------------
c     Figure out how many lines are printed out for each year
c-----------------------------------------------------------------------
      nline=Ny/Tblcol
      IF(mod(Ny,Tblcol).gt.0)nline=nline+1
c-----------------------------------------------------------------------
c     Print the complete table (column header, I, S, MSR) for each 
c     of the sets of months used in the main printout.
c-----------------------------------------------------------------------
      DO n=1,nline
       n1=(n-1)*Tblcol+1
       n2=n*Tblcol
       IF(n2.gt.Ny)n2=Ny
c-----------------------------------------------------------------------
c     Create column headings
c-----------------------------------------------------------------------
       IF(Ny.eq.4)THEN
        l=5
       ELSE
        l=13
       END IF
       CALL prtcol(l,n,Tblcol,Tblwid,Ny,Mt1,5,'     ',Disp2,Disp3,
     &             Fmtcol,Colhdr)
c-----------------------------------------------------------------------
c     Generate the output format
c-----------------------------------------------------------------------
      if(Tblwid.gt.9)then
       write(fobs,1010)Tblwid
 1010  format('f',i2,'.3')
       ifmt=5
      else
       write(fobs,1020)Tblwid
 1020  format('f',i1,'.3')
       ifmt=4
      end if
      write(fsum,1010)Tblwid+2
      fbase=' '
      CALL getstr(TFMDIC,tfmptr,PTFM,Iptr+1,fbase,ipos)
      IF(Lfatal)RETURN
      CALL cnvfmt(fbase,tfmt,fobs(1:ifmt),fsum,ipos,npos)
c-----------------------------------------------------------------------
c     Print out variance of irregular component
c-----------------------------------------------------------------------
       n3=n2-n1+1
       DO i=n1,n2
        tmp(i-n1+1)=Rati(i)
       END DO
       CALL wrttbl(tmp,0,'  I  ',n3,3,Mt1,tfmt(1:npos),Tblwid,Disp1,
     &             Disp2,Disp3,n3,0,0,0,F)
       IF(Lfatal)RETURN
c-----------------------------------------------------------------------
c     Print out variance of Seasonal component
c-----------------------------------------------------------------------
       DO i=n1,n2
        tmp(i-n1+1)=Rati(i+Ny)
       END DO
       CALL wrttbl(tmp,0,'  S  ',n3,3,Mt1,tfmt(1:npos),Tblwid,Disp1,
     &             Disp2,Disp3,n3,0,0,0,F)
       IF(Lfatal)RETURN
c-----------------------------------------------------------------------
c     Print out Moving Seasonality Ratio
c-----------------------------------------------------------------------
       DO i=n1,n2
        tmp(i-n1+1)=Rati(i+2*Ny)
       END DO
       CALL wrttbl(tmp,0,'RATIO',n3,3,Mt1,tfmt(1:npos),Tblwid,Disp1,
     &             Disp2,Disp3,n3,0,0,0,F)
       IF(Lfatal)RETURN
c-----------------------------------------------------------------------
       WRITE(Mt1,1030)
 1030  FORMAT(//)
      END DO
      RETURN
      END
