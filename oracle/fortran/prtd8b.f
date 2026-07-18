C     Last change:  BCM  17 Apr 2003   11:10 pm
      SUBROUTINE prtd8b(Stsie,Stwt,Pos1ob,Posfob,Tblptr,Lprt,Lsav,Lgraf)
      IMPLICIT NONE
c-----------------------------------------------------------------------
C --- THIS SUBROUTINE GENERATES a new table of SI ratios with labels for
c     C17 extreme values, regARIMA outliers.
c-----------------------------------------------------------------------
      DOUBLE PRECISION ONE,ZERO
      LOGICAL F,T
      PARAMETER(F=.false.,T=.true.,ONE=1D0,ZERO=0D0)
c-----------------------------------------------------------------------
      INCLUDE 'stdio.i'
      INCLUDE 'srslen.prm'
      INCLUDE 'model.prm'
      INCLUDE 'model.cmn'
      INCLUDE 'arima.cmn'
      INCLUDE 'mdldat.cmn'
      INCLUDE 'extend.cmn'
      INCLUDE 'x11adj.cmn'
      INCLUDE 'tfmts.cmn'
      INCLUDE 'units.cmn'
      INCLUDE 'hiddn.cmn'
      INCLUDE 'error.cmn'
      INCLUDE 'title.cmn'
      INCLUDE 'x11opt.cmn'
      INCLUDE 'notset.prm'
      INCLUDE 'tbltitle.prm'
c-----------------------------------------------------------------------
c     Include data dictionary of table formats
c-----------------------------------------------------------------------
      INCLUDE 'tfmts.prm'
      INCLUDE 'tfmts2.prm'
c-----------------------------------------------------------------------
      CHARACTER extchr*(2),str*(PCOLCR),ifmt1a*(110),ifmt2a*(110),
     &          fbase*(110),fobs*(5),fsum*(5),tblttl*(PTTLEN),ctmp*(2),
     &          fmtcl2*(110),keylbl*(4),outstr*(400),colon*(2)
      DOUBLE PRECISION Stsie,Stwt,dvec,tmp
      LOGICAL locok,Lgraf,Lprt,Lsav,lfac
      INTEGER Pos1ob,Posfob,extind,i,nchr,otltyp,begotl,endotl,nmod,
     &        ndsp,nbk,nbk2,icol,l,ipos,ifmt,ntbttl,jyr,kyr,ipow,npos,
     &        ldec,im,ib1,ie1,idate,nfmt1a,nfmt2a,Tblptr,tw2,im0,im1,
     &        im2,im3,numobs,j,nc
      DIMENSION Stsie(PLEN),Stwt(PLEN),extind(PLEN),extchr(PLEN),
     &          tmp(PSP+1),ctmp(PSP+1),dvec(1),idate(2)
c-----------------------------------------------------------------------
      LOGICAL dpeq
      INTEGER numaff
      EXTERNAL dpeq,numaff
c-----------------------------------------------------------------------
      INCLUDE 'tfmts.var'
      INCLUDE 'tfmts2.var'
c-----------------------------------------------------------------------
c        INITIALIZE variables
c-----------------------------------------------------------------------
      CALL setint(0,PLEN,extind)
      keylbl='    '
      ldec=Kdec
      ipow=0
      IF(Muladd.ne.1)THEN
       IF(Kdec.eq.0)ldec=1
       ipow=1
      END IF
      DO i=Pos1ob,Posfob
       extchr(i)='  '
      END DO
      nmod=5
      IF(Ny.lt.5)nmod=10
c-----------------------------------------------------------------------
c     Determine which observations are revised as X-11 "extreme" values
c-----------------------------------------------------------------------
      DO i = Pos1ob,Posfob
       IF(.not.dpeq(Stwt(i),ONE))THEN
        extind(i)=extind(i)+1
        IF(keylbl(1:1).eq.' ')keylbl(1:1)='*'
       END IF
      END DO
c-----------------------------------------------------------------------
c     Determine which observations are revised by regARIMA outliers
c-----------------------------------------------------------------------
      im0=0
      IF((Adjls.eq.1.and.(Nls.gt.0.or.Nramp.gt.0)).or.(Nao.gt.0.and.
     &    Adjao.eq.1).or.(Adjtc.eq.1.and.Ntc.gt.0))THEN
       DO icol=1,Nb
        IF((Adjao.eq.1.and.
     &     (Rgvrtp(icol).eq.PRGTAO.or.Rgvrtp(icol).eq.PRGTAA)).or.
     &     (Adjtc.eq.1.and.
     &     (Rgvrtp(icol).eq.PRGTTC.or.Rgvrtp(icol).eq.PRGTAT)).or.
     &     (Adjls.eq.1.and.
     &     (Rgvrtp(icol).eq.PRGTLS.or.Rgvrtp(icol).eq.PRGTAL.or.
     &      Rgvrtp(icol).eq.PRGTRP.or.Rgvrtp(icol).eq.PRGTTL.or.
     &      Rgvrtp(icol).eq.PRGTQD.or.Rgvrtp(icol).eq.PRGTQI)))THEN
c-----------------------------------------------------------------------
c     Get regARIMA outlier information
c-----------------------------------------------------------------------
         CALL getstr(Colttl,Colptr,Ncoltl,icol,str,nchr)
         IF(Lfatal)RETURN
         CALL rdotlr(str(1:nchr),Begxy,Sp,otltyp,begotl,endotl,locok)
         IF(.not.locok)THEN
          CALL abend
          RETURN
         END IF
c-----------------------------------------------------------------------
c     Determine which observation(s) are effected by these outliers
c-----------------------------------------------------------------------
         CALL dfdate(Begxy,Begbk2,Sp,ndsp)
         i=begotl+ndsp
         extind(i)=extind(i)+2
         IF(otltyp.eq.RP)THEN
          DO i=begotl+ndsp+1,endotl+ndsp
           extind(i)=extind(i)+2
          END DO
         ELSE IF(otltyp.eq.LS)THEN
          im1=numaff(B(icol),Muladd,Nterm)
          IF(im1.gt.0)THEN
           DO im2=1,im1
            im3=i+im2
            IF(im3.le.Posfob)extchr(im3)(1:2)=' -'
            im3=i-im2
            IF(im3.ge.Pos1ob)extchr(im3)(1:2)=' -'
           END DO
           im0=im0+im1
          END IF
         END IF
        END IF
       END DO
      END IF
c-----------------------------------------------------------------------
c     Create a string of labels to indicate which observations are 
c     effected by the different types of outlier adjustment
c-----------------------------------------------------------------------
      DO i=Pos1ob,Posfob
       IF(extind(i).gt.0)THEN
        IF(extind(i).eq.1)THEN
         extchr(i)(1:1)='*'
        ELSE IF(extind(i).eq.2)THEN
         extchr(i)(1:1)='#'
         IF(keylbl(2:2).eq.' ')keylbl(2:2)='#'
        ELSE IF(mod(extind(i),2).eq.0)THEN
         extchr(i)(1:1)='@'
         IF(keylbl(3:3).eq.' ')keylbl(3:3)='@'
        ELSE
         extchr(i)(1:1)='&'
         IF(keylbl(4:4).eq.' ')keylbl(4:4)='&'
        END IF
       END IF
      END DO
      IF(Lprt)THEN
c-----------------------------------------------------------------------
c     Create formats for printing out the table
c-----------------------------------------------------------------------
       IF(Tblwid.gt.9)then
        write(fobs,1010)Tblwid,ldec
 1010   FORMAT('f',i2,'.',i1)
        ifmt=5
       ELSE
        write(fobs,1020)Tblwid,ldec
 1020   FORMAT('f',i1,'.',i1)
        ifmt=4
       ENDIF
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
       tw2=Tblwid+1
       if(tw2.gt.9)then
        write(fobs,1030)tw2
 1030   FORMAT('a',i2)
        ifmt=3
       else
        write(fobs,1040)tw2
 1040   FORMAT('a',i1)
        ifmt=2
       endif
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
       numobs=Posfob-Pos1ob+1
       CALL tblhdr(8,0,Ixreg,numobs,Begspn,Ny,dvec,tblttl(1:ntbttl))
       IF(Lfatal)RETURN
       IF(Ny.eq.4)THEN
        l=5
       ELSE
        l=13
       END IF
       CALL prtcol(l,0,Tblcol,tw2,Ny,Mt1,5,'     ',Disp2,Disp3,fmtcl2,
     &             Colhdr)
       l=l-1
c-----------------------------------------------------------------------
c     print out table
c-----------------------------------------------------------------------
       jyr=Lyr+(Pos1ob-1)/Ny
       kyr=(Posfob+Ny-1)/Ny+Lyr-1
c       iin=iin+(jyr-Lyr)
       DO i=1,13
        tmp(i)=DNOTST
        ctmp(i)='  '
       END DO
       ib1=Pos1ob
       ie1=(jyr-Lyr+1)*Ny
       IF(ie1.gt.Posfob)ie1=Posfob
       im=Pos1ob-(Pos1ob-1)/Ny*Ny
       DO WHILE (T)
        im1=im
        DO i=ib1,ie1
         IF(Muladd.eq.2)THEN
          tmp(im)=exp(Stsie(i))
         ELSE
          tmp(im)=Stsie(i)
         END IF
         ctmp(im)=extchr(i)
         im=im+1
        END DO
        im2=im-1
c-----------------------------------------------------------------------
c     Compute number of blanks for the beginning or end of the series
c     for observations not in the series.
c-----------------------------------------------------------------------
        nbk=0
        IF(jyr.eq.Begspn(YR).and.Begspn(MO).gt.1)nbk=Begspn(MO)
        nbk2=0
        IF(ie1.eq.Posfob)THEN
         CALL addate(Begspn,Ny,numobs-1,idate)
         nbk2=idate(MO)
         IF(nbk2.eq.Ny)nbk2=0
        END IF
c-----------------------------------------------------------------------
c     Write out this year's data.
c-----------------------------------------------------------------------
        CALL wrttb2(tmp,ctmp,jyr,'XXXXX',l,ldec,Mt1,ifmt1a(1:nfmt1a),
     &              tw2,Tblcol,Disp1,Disp2,Disp3,nbk,nbk2,ipow,1,
     &              l.eq.13.or.l.eq.5)
        IF(Lfatal)RETURN
        IF((.not.Lcmpaq).or.
     &     (kyr.lt.jyr+1 .or. (mod(jyr,nmod)+1).eq.nmod))WRITE(Mt1,1050)
 1050   FORMAT(' ')
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
         ie1=Posfob
        ELSE IF(kyr.lt.jyr)THEN
         GO TO 10
        END IF
       END DO
c-----------------------------------------------------------------------
 10    IF(keylbl.eq.'    ')THEN
        WRITE(Mt1,1060)
 1060   FORMAT(/,'  No extreme values or regARIMA outliers.')
       ELSE
        IF(Lcmpaq)THEN
         WRITE(Mt1,1071)
        ELSE
         WRITE(Mt1,1070)
        END IF
 1070   FORMAT(/,'  Key to symbols:')
 1071   FORMAT('  Key to symbols:')
        DO i=1,4
         IF(keylbl(i:i).eq.'*')THEN
          WRITE(Mt1,1080)
 1080     FORMAT('    *  : extreme value as determined by X-11 extreme',
     &           ' value procedure')
         ELSE IF(keylbl(i:i).eq.'#')THEN
          WRITE(Mt1,1090)
 1090     FORMAT('    #  : regARIMA outlier (either AO, LS, TC, or ',
     &           'Ramp)')
         ELSE IF(keylbl(i:i).eq.'@')THEN
          WRITE(Mt1,1100)
 1100     FORMAT('    @  : extreme value and at least one type of ',
     &           'regARIMA outlier')
         ELSE IF(keylbl(i:i).eq.'&')THEN
          WRITE(Mt1,1110)
 1110     FORMAT('    &  : more than one type of regARIMA outlier')
         END IF
        END DO
       END IF
       IF((Adjls.eq.1.and.Nls.gt.0).and.im0.gt.0)WRITE(Mt1,1120)
 1120  FORMAT('    -  : values around a level shift most likely to be',
     &          ' influenced by it')
       IF(.not.Lcmpaq)WRITE(Mt1,1050)
      END IF
c-----------------------------------------------------------------------
      IF(Lsav.or.Lgraf)THEN
       IF(Lsav)
     &    CALL savd8b(Tblptr,Begbk2,Pos1ob,Posfob,Ny,Stsie,extchr,Serno,
     &                Nser,F)
       IF(.not.Lfatal.and.Lgraf)
     &    CALL savd8b(Tblptr,Begbk2,Pos1ob,Posfob,Ny,Stsie,extchr,Serno,
     &                Nser,Lgraf)
       IF(Lfatal)RETURN
      END IF
c-----------------------------------------------------------------------
      IF(Lsumm.gt.0)THEN
       DO i=Pos1ob,Pos1ob+Ny-1
        ipos=1
        nc=1
        colon=': '
        DO j=i,Posfob,Ny
         CALL addate(Begbk2,Ny,j-1,idate)
         IF(.not.(extchr(j).eq.'  '))THEN
          CALL itoc(idate(YR),outstr,ipos)
          IF(Lfatal)RETURN
          IF(.not.(extchr(j)(1:1).eq.' '))THEN
           IF(extchr(j)(1:1).eq.'*')THEN
            IF(dpeq(Stwt(j),ZERO))THEN
             outstr(ipos:ipos)='z'
            ELSE
             outstr(ipos:ipos)=extchr(j)(1:1)
            END IF
            ipos=ipos+1
           ELSE
            outstr(ipos:ipos)=extchr(j)(1:1)
            ipos=ipos+1
            IF(dpeq(Stwt(j),ZERO))THEN
             outstr(ipos:ipos)='z'
             ipos=ipos+1
            END IF
           END IF
          END IF
          IF(.not.(extchr(j)(2:2).eq.' '))THEN
           outstr(ipos:ipos)=extchr(j)(2:2)
           ipos=ipos+1
          END IF
          outstr(ipos:ipos)=' '
          ipos=ipos+1
          IF(ipos.ge.PFILCR)THEN
           WRITE(Nform,2000)idate(MO),colon(1:nc),outstr(1:(ipos-1))
           ipos=1
           nc=2
           colon='c:'
          END IF
         END IF
        END DO
        IF(ipos.gt.1)THEN
         WRITE(Nform,2000)idate(MO),colon(1:nc),outstr(1:(ipos-1))
        ELSE
         WRITE(Nform,2000)idate(MO),colon(1:nc),'none'
        END IF
 2000   FORMAT('d8b.',i2.2,a,1x,a)
       END DO
      END IF
c-----------------------------------------------------------------------
      RETURN
      END
