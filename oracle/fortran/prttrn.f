      SUBROUTINE prttrn(Stc,Trnchr,Ib,Ie,Ktabl,Tblptr)
      IMPLICIT NONE
c-----------------------------------------------------------------------
C --- THIS SUBROUTINE GENERATES a table of the trend component with 
c     labels for observations that were replaced because they were
c     less than zero (for a multipicative seasonal adjustment).
c-----------------------------------------------------------------------
      LOGICAL F,T
      PARAMETER(F=.false.,T=.true.)
c-----------------------------------------------------------------------
      INCLUDE 'stdio.i'
      INCLUDE 'srslen.prm'
      INCLUDE 'model.prm'
      INCLUDE 'tbltitle.prm'
      INCLUDE 'notset.prm'
      INCLUDE 'error.cmn'
      INCLUDE 'hiddn.cmn'
      INCLUDE 'title.cmn'
      INCLUDE 'tfmts.cmn'
      INCLUDE 'units.cmn'
      INCLUDE 'x11ptr.cmn'
      INCLUDE 'x11opt.cmn'
c-----------------------------------------------------------------------
c     Include data dictionary of table formats
c-----------------------------------------------------------------------
      INCLUDE 'tfmts.prm'
      INCLUDE 'tfmts2.prm'
c-----------------------------------------------------------------------
      CHARACTER fsum*(5),Trnchr*(1),fobs*(5),tblttl*(PTTLEN),
     &          ifmt1a*(110),ifmt2a*(110),fbase*(110),fmtcl2*(110),
     &          ctmp*(1)
      DOUBLE PRECISION Stc,tmp,xmin,xmax,dvec
      INTEGER Ib,Ie,Tblptr,iopt,Ktabl,tw2,idate,begtbl,ipos,ntbttl,l,im,
     &        im1,im2,jyr,kyr,nbk,nbk2,i,ldec,ipow,ifmt,nfmt1a,nfmt2a,
     &        npos,nobs,ib1,ie1
      DIMENSION Stc(PLEN),Trnchr(PLEN),tmp(PSP+1),ctmp(PSP+1),dvec(1),
     &          idate(2),begtbl(2)
c-----------------------------------------------------------------------
      DOUBLE PRECISION totals,sdev
      EXTERNAL totals,sdev
c-----------------------------------------------------------------------
c     include files containing DATA statements 
c-----------------------------------------------------------------------
      INCLUDE 'tfmts.var'
      INCLUDE 'tfmts2.var'
c-----------------------------------------------------------------------
c        INITIALIZE variables
c-----------------------------------------------------------------------
      ldec=Kdec
      ipow=0
      iopt=0
      DO i=Ib,Ie
       IF(i.eq.Ib)THEN
        xmin=Stc(i)
        xmax=Stc(i)
       ELSE
        IF(xmin.gt.Stc(i))xmin=Stc(i)
        IF(xmax.lt.Stc(i))xmax=Stc(i)
       END IF
      END DO
c-----------------------------------------------------------------------
c     Create formats for printing out the table
c-----------------------------------------------------------------------
      IF(Tblwid.gt.9)then
       write(fobs,1010)Tblwid,ldec
 1010  FORMAT('f',i2,'.',i1)
       ifmt=5
      ELSE
       write(fobs,1020)Tblwid,ldec
 1020  FORMAT('f',i1,'.',i1)
       ifmt=4
      end if
      write(fsum,1010)Tblwid+2,ldec
      CALL setchr(' ',110,fbase)
      CALL getstr(TF2DIC,tf2ptr,PTF2,Iptr,fbase,ipos)
      IF(Lfatal)RETURN
      CALL cnvfmt(fbase,ifmt1a,fobs(1:ifmt),fsum,ipos,nfmt1a)
      CALL setchr(' ',110,fbase)
      CALL getstr(TF2DIC,tf2ptr,PTF2,Iptr+1,fbase,ipos)
      IF(Lfatal)RETURN
      CALL cnvfmt(fbase,ifmt2a,fobs(1:ifmt),fsum,ipos,nfmt2a)
c-----------------------------------------------------------------------
c     Construct revised format for column headings.
c-----------------------------------------------------------------------
c      tw2=Tblwid+1
      tw2=Tblwid
      if(tw2.gt.9)then
       write(fobs,1030)tw2
 1030  FORMAT('a',i2)
       ifmt=3
      else
       write(fobs,1040)tw2
 1040  FORMAT('a',i1)
       ifmt=2
      end if
      write(fsum,1030)tw2+2
      CALL setchr(' ',110,fbase)
      CALL getstr(TFMDIC,tfmptr,PTFM,Iptr,fbase,ipos)
      IF(Lfatal)RETURN
      CALL cnvfmt(fbase,fmtcl2,fobs(1:ifmt),fsum(1:3),ipos,npos)
      fmtcl2(1:npos)=Ifmt2(1:6)//fmtcl2(7:npos)
c-----------------------------------------------------------------------
c     Generate headers and subheaders for the table
c-----------------------------------------------------------------------
      CALL getdes(Tblptr,tblttl,ntbttl,T)
      IF(Lfatal)RETURN
      nobs=Ie-Ib+1
      begtbl(YR)=Lyr
      begtbl(MO)=mod(Ib,Ny)
      IF(begtbl(MO).eq.0)begtbl(MO)=Ny
      IF(Ib.gt.Pos1bk)begtbl(YR)=begtbl(YR)+((Ib-1)/Ny)-((Pos1bk-1)/Ny)
      CALL tblhdr(Ktabl,0,Ixreg,nobs,begtbl,Ny,dvec,tblttl(1:ntbttl))
      IF(Lfatal)RETURN
      IF(Ny.eq.4)THEN
       l=5
      ELSE
       l=13
      END IF
      CALL prtcol(l,0,Tblcol,tw2,Ny,Mt1,2,'TOTAL',Disp2,Disp3,fmtcl2,
     &            Colhdr)
c-----------------------------------------------------------------------
c     print out table
c-----------------------------------------------------------------------
      jyr=Lyr+(Ib-1)/Ny
      kyr=(Ie+Ny-1)/Ny+Lyr-1
*      iin=iin+(jyr-Lyr)
      DO i=1,PSP+1
       tmp(i)=DNOTST
       ctmp(i)=' '
      END DO
      ib1=Ib
      ie1=(jyr-Lyr+1)*Ny
      IF(ie1.gt.Ie)ie1=Ie
      im=Ib-(Ib-1)/Ny*Ny
      DO WHILE (T)
       im1=im
       DO i=ib1,ie1
        tmp(im)=Stc(i)
        ctmp(im)=Trnchr(i)
        im=im+1
       END DO
       im2=im-1
       tmp(l)=totals(tmp,im1,im2,1,0)
c-----------------------------------------------------------------------
c     Compute number of blanks for the beginning or end of the series
c     for observations not in the series.
c-----------------------------------------------------------------------
       nbk=0
       IF(jyr.eq.begtbl(YR).and.begtbl(MO).gt.1)nbk=begtbl(MO)
       nbk2=0
       IF(ie1.eq.Ie)THEN
        CALL addate(begtbl,Ny,nobs-1,idate)
        nbk2=idate(MO)
        IF(nbk2.eq.Ny)nbk2=0
       END IF
c-----------------------------------------------------------------------
c     Write out this year's data.
c-----------------------------------------------------------------------
       CALL wrttb2(tmp,ctmp,jyr,'XXXXX',l,ldec,Mt1,ifmt1a(1:nfmt1a),
     &             tw2,Tblcol,Disp1,Disp2,Disp3,nbk,nbk2,ipow,0,
     &             l.eq.13.or.l.eq.5)
       IF(Lfatal)RETURN
       WRITE(Mt1,1050)
 1050  FORMAT(' ')
c-----------------------------------------------------------------------
c     Update year, starting and ending position of year
c-----------------------------------------------------------------------
       jyr=jyr+1
       im=1
       ib1=ie1+1
       ie1=ie1+Ny
       IF(kyr.eq.jyr)THEN
        DO i=1,Ny
         tmp(i)=DNOTST
        END DO
        ie1=Ie
       ELSE IF(kyr.lt.jyr)THEN
        ie1=Ib+Ny-1
        im=Ib-(Ib-1)/Ny*Ny
        nbk=0
        nbk2=0
        DO i=Ib,ie1
         ctmp(im)=' '
         IF(i.gt.Ie)THEN
          tmp(im)=DNOTST
          IF(i.eq.im)THEN
           IF(nbk2.eq.0)nbk2=im-1
          ELSE
           IF(nbk.eq.0)nbk=1
           nbk=nbk+1
          END IF
         ELSE
          tmp(im)=totals(Stc,i,Ie,Ny,1)
         END IF
         IF(im.eq.Ny)im=0
         im=im+1
        END DO
C --- GENERATE COLUMN SUMMARY FORMATS.
c02      WRITE(MT1,IF2) TYRLY(NOP1),(TMP(I),I = 1,NY)
        CALL wrttb2(tmp,ctmp,0,'AVGE ',Ny,ldec,Mt1,ifmt2a(1:nfmt2a),
     &              tw2,Tblcol,Disp1,Disp2,Disp3,nbk,nbk2,ipow,0,F)
        IF(Lfatal)RETURN
        tmp(1)=totals(Stc,Ib,Ie,1,0)
        tmp(2)=tmp(1)/dble(Ie-Ib+1)
        tmp(3)=sdev(Stc,Ib,Ie,1,iopt)
C --- WRITE TABLE SUMMARY.
        WRITE(Mt1,Ifmt3)(tmp(i),i=1,3),xmin,xmax
        GO TO 10
       END IF
      END DO
c-----------------------------------------------------------------------
   10 IF(Lwdprt)THEN
       WRITE(Mt1,1060)PRGNAM
      ELSE
       WRITE(Mt1,1070)PRGNAM
      END IF
 1060 FORMAT(//,'         * - Trend cycle estimate that had a negative',
     &          ' value replaced by ',a,'.')
 1070 FORMAT(//,'   * - Trend cycle estimate that had a negative ',
     &          'value replaced by',/,'       ',a,'.')
c-----------------------------------------------------------------------
      RETURN
      END
