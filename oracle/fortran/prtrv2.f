C     Last change:  BCM  16 Feb 1999    3:50 pm
      SUBROUTINE prtrv2(Fin,Cnc,Cnc2,Revspn,Nptr,Lsumm,Lgraf)
c-----------------------------------------------------------------------
c     Print and/or save a table of the percent revision, concurrent
c     and final value of the regular (cnc) and projected (cnc2) seasonal
c     factors.
c-----------------------------------------------------------------------
      IMPLICIT NONE
c-----------------------------------------------------------------------
      INCLUDE 'notset.prm'
      INCLUDE 'srslen.prm'
      INCLUDE 'rev.prm'
      INCLUDE 'rev.cmn'
      INCLUDE 'error.cmn'
      INCLUDE 'tbllog.prm'
      INCLUDE 'tbllog.cmn'
      INCLUDE 'x11msc.cmn'
      INCLUDE 'svllog.prm'
      INCLUDE 'svllog.cmn'
      INCLUDE 'dgnsvl.i'
      INCLUDE 'units.cmn'
      INCLUDE 'cchars.i'
      INCLUDE 'tfmts.cmn'
      INCLUDE 'x11opt.cmn'
c-----------------------------------------------------------------------
      INCLUDE 'tbltitle.prm'
      INCLUDE 'desdgn.prm'
c-----------------------------------------------------------------------
      LOGICAL F,T
      INTEGER MO,YR,PCOLRV,PNCHDR
      PARAMETER(MO=2,YR=1,F=.false.,T=.true.,PCOLRV=23,PNCHDR=6)
c-----------------------------------------------------------------------
      CHARACTER cobs*(13),cpobs*(3),tfmt1*(110),tblttl*(PTTLEN),
     &          hdrttl*(PCOLRV*PNCHDR),hd2ttl*(PCOLRV*PNCHDR),
     &          tfmt2*(110),outstr*(6+(23*3)),brklbl*(7)
      DOUBLE PRECISION Cnc,Cnc2,Fin,rev,tmp,trev,aarpd,aaryr,aartot,
     &                 narpd,naryr,nartot,ts,revhng,drv,xtmp
      INTEGER i,j,Lsumm,Revspn,fh,fh2,Nptr,k,k2,i3,ielt,ipos,
     &        idate,ntbttl,ncol,i2,i0,npos,
     &        iper,hdrptr,hd2ptr,rdbdat,nhdr,nhd2,iyr,
     &        nstr,tbw,end2,k3,nhdrtl,nhd2tl
      LOGICAL Lgraf,locok
      DIMENSION Cnc(PREV),Cnc2(PREV),Fin(PREV),rev(0:1,PREV),Revspn(2),
     &          trev(PREV+12),idate(2),tmp(0:5,PREV),ts(5),cpobs(16),
     &          aarpd(0:1,12),aaryr(0:1,PREVY),aartot(0:1,1),
     &          narpd(0:1,12),naryr(0:1,PREVY),nartot(0:1,1),
     &          hdrptr(0:PNCHDR),hd2ptr(0:PNCHDR),revhng(0:1,5)
c-----------------------------------------------------------------------
      LOGICAL dpeq
      EXTERNAL dpeq
c-----------------------------------------------------------------------
      INCLUDE 'desdgn.var'
      DATA cpobs/'Jan','Feb','Mar','Apr','May','Jun','Jul','Aug','Sep',
     &           'Oct','Nov','Dec','1st','2nd','3rd','4th'/
c-----------------------------------------------------------------------
c     Set logical variable to determine if percent revisions are to be 
c     printed out
c-----------------------------------------------------------------------
      Rvper=T
      IF(Muladd.eq.1)Rvper=F
      IF(Lsumm.gt.0)THEN
       IF(Rvper)THEN
        WRITE(Nform,1010)'percent'
       ELSE
        WRITE(Nform,1010)'difference'
       END IF
 1010  FORMAT('r06.aarmode: ',a)
      END IF
c-----------------------------------------------------------------------
c     Initialize values for printing tables
c-----------------------------------------------------------------------
      ncol=1
c-----------------------------------------------------------------------
c     Generate table format
c-----------------------------------------------------------------------
      IF(Prttab(Nptr).or.Prttab(Nptr+1))THEN
       npos=21
       WRITE(tfmt1,1020)ncol+1
 1020  FORMAT('(1x,a5,1x,',i1,'(1x,f9.2))')
       WRITE(tfmt2,1030)ncol+1
 1030  FORMAT('(2x,i4,1x,',i1,'(1x,f9.2))')
c-----------------------------------------------------------------------
c     create column headers
c-----------------------------------------------------------------------
       CALL intlst(PNCHDR,hdrptr,nhdrtl)
       nhdr=nhdrtl+1
       CALL insstr('Proj -',nhdr,PNCHDR,hdrttl,hdrptr,nhdrtl)
       IF(Lfatal)RETURN
       CALL insstr('Conc -',nhdr,PNCHDR,hdrttl,hdrptr,nhdrtl)
       IF(Lfatal)RETURN
       CALL intlst(PNCHDR,hd2ptr,nhd2tl)
       nhd2=nhd2tl+1
       DO i2=1,2
        CALL insstr('Final ',nhd2,3,hd2ttl,hd2ptr,nhd2tl)
        IF(Lfatal)RETURN
       END DO
      END IF
c-----------------------------------------------------------------------
c     Intitalize variables for summary tables
c-----------------------------------------------------------------------
      iper=Revspn(MO)-1
      iyr=1
      CALL setdp(0D0,24,aarpd)
      CALL setdp(0D0,2*PREVY,aaryr)
      CALL setdp(0D0,2,aartot)
      CALL setdp(0D0,24,narpd)
      CALL setdp(0D0,2*PREVY,naryr)
      CALL setdp(0D0,2,nartot)
c-----------------------------------------------------------------------
c     Compute revision for given estimate
c-----------------------------------------------------------------------
      DO i=Begrev,Endsa-1
       Revptr=i-Begrev+1
c-----------------------------------------------------------------------
c     Set indexes for summary tables
c-----------------------------------------------------------------------
       iper=iper+1
       IF(iper.gt.Ny)THEN
        iper=1
        iyr=iyr+1
       END IF
c-----------------------------------------------------------------------
c     Calculate the (percent) revision between the concurrent and final
c     adjustments.
c-----------------------------------------------------------------------
       rev(0,Revptr)=Fin(Revptr)-Cnc(Revptr)
       IF(Rvper)rev(0,Revptr)=(rev(0,Revptr)/Cnc(Revptr))*100D0
       rev(1,Revptr)=Fin(Revptr)-Cnc2(Revptr)
       IF(Rvper)rev(1,Revptr)=(rev(1,Revptr)/Cnc2(Revptr))*100D0
c-----------------------------------------------------------------------
c     Keep track of summary statistics
c-----------------------------------------------------------------------
       IF(Prttab(Nptr+1).or.Lsumm.gt.0.or.Svltab(LSLASF).or.
     &    Svltab(LSLASP))THEN
        DO i2=0,1
         drv=dabs(rev(i2,Revptr))
         aarpd(i2,iper)=aarpd(i2,iper)+drv
         narpd(i2,iper)=narpd(i2,iper)+1D0
         aaryr(i2,iyr)=aaryr(i2,iyr)+drv
         naryr(i2,iyr)=naryr(i2,iyr)+1D0
         aartot(i2,1)=aartot(i2,1)+drv
         nartot(i2,1)=nartot(i2,1)+1D0
        END DO
       END IF
      END DO
      IF(Prttab(Nptr))THEN
c-----------------------------------------------------------------------
c     First, print out header information
c-----------------------------------------------------------------------
       CALL makttl(DSDDIC,dsdptr,PDSD,Nptr,PDSUM6,tblttl,ntbttl,T,F)
       IF(.not.Lfatal)CALL prtshd(tblttl(1:ntbttl),Revspn,Ny,Revnum,T)
       IF(Lfatal)RETURN
c-----------------------------------------------------------------------
c     Print out table
c-----------------------------------------------------------------------
       i0=0
       j=Revspn(YR)-1
       i=Begrev
       i2=Begrev+Ny-Revspn(MO)
       i3=Revspn(MO)
       DO WHILE (i.lt.Endsa)
        IF(i2.ge.Endsa)i2=Endsa-1
        j=j+1
        ipos=1
        CALL itoc(j,cobs,ipos) 
        CALL prrvob(rev,ncol,i-Begrev+1,i2-Begrev+1,1,1,
     &              cobs,ipos-1,hdrttl,hdrptr,nhdrtl,hd2ttl,
     &              hd2ptr,nhd2tl,i0,tfmt1(1:npos),i3,1,PNCHDR,9,
     &              tblttl(1:ntbttl),F)
        IF(Lfatal)RETURN
        IF(i.eq.Begrev)i3=1
        i=i2+1
        i2=i2+Ny
       END DO
      END IF
c-----------------------------------------------------------------------
c     print out summary tables, if necessary.
c-----------------------------------------------------------------------
      IF(Prttab(Nptr+1))THEN
c-----------------------------------------------------------------------
c     First, print out header information
c-----------------------------------------------------------------------
       CALL makttl(DSDDIC,dsdptr,PDSD,Nptr+1,PDSUM6,tblttl,ntbttl,T,F)
       IF(.not.Lfatal)CALL prtshd(tblttl(1:ntbttl),Revspn,Ny,0,T)
       IF(Lfatal)RETURN
      END IF
c-----------------------------------------------------------------------
c     Compute summary statistics
c-----------------------------------------------------------------------
      IF(Prttab(Nptr+1).or.Lsumm.gt.0.or.Svltab(LSLASF).or.
     &   Svltab(LSLASP))THEN
       DO j=0,ncol
        IF(dpeq(nartot(j,1),0D0))THEN
         aartot(j,1)=DNOTST
        ELSE
         aartot(j,1)=aartot(j,1)/nartot(j,1)
        END IF
        DO i=1,max(Ny,iyr)
         IF(i.le.Ny)THEN
          IF(dpeq(narpd(j,i),0D0))THEN
           aarpd(j,i)=DNOTST
          ELSE
           aarpd(j,i)=aarpd(j,i)/narpd(j,i)
          END IF
         END IF
         IF(i.le.iyr)THEN
          IF(dpeq(naryr(j,i),0D0))THEN
           aaryr(j,i)=DNOTST
          ELSE
           aaryr(j,i)=aaryr(j,i)/naryr(j,i)
          END IF
         END IF
        END DO
       END DO
      END IF
c-----------------------------------------------------------------------
c     First, print out absolute average for each period.
c-----------------------------------------------------------------------
      IF(Prttab(Nptr+1))THEN
       i0=0
       IF(Ny.eq.12)THEN
        cobs='Months:      '
        nstr=7
       ELSE
        cobs='Quarters:    '
        nstr=9
       END IF
       CALL prrvob(aarpd,ncol,1,Ny,1,1,cobs,nstr,hdrttl,hdrptr,
     &             nhdrtl,hd2ttl,hd2ptr,nhd2tl,i0,tfmt1(1:npos),1,1,
     &             PNCHDR,9,tblttl(1:ntbttl),T)
       IF(Lfatal)RETURN
c-----------------------------------------------------------------------
c     Then, print out absolute average for each year.
c-----------------------------------------------------------------------
       cobs='Years:       '
       nstr=6
       CALL prrvob(aaryr,ncol,1,iyr,1,2,cobs,nstr,hdrttl,hdrptr,
     &             nhdrtl,hd2ttl,hd2ptr,nhd2tl,i0,tfmt2(1:npos),
     &             Revspn(YR)-1,1,PNCHDR,9,tblttl(1:ntbttl),T)
       IF(Lfatal)RETURN
c-----------------------------------------------------------------------
c     Finally, print out the total absolute average.
c-----------------------------------------------------------------------
       cobs='Total:       '
       CALL prrvob(aartot,ncol,1,1,1,0,cobs,nstr,hdrttl,hdrptr,
     &             nhdrtl,hd2ttl,hd2ptr,nhd2tl,i0,tfmt1(1:npos),
     &             Revspn(YR)-1,1,PNCHDR,9,tblttl(1:ntbttl),T)
       IF(Lfatal)RETURN
      END IF
c-----------------------------------------------------------------------
c     Compute hinge statistics for the absolute revisions
c-----------------------------------------------------------------------
      IF(Prttab(Nptr+1).or.Lsumm.gt.0)THEN
       DO i=0,ncol
        i2=0
        DO k=1,INT(nartot(i,1))
         i2=i2+1
         trev(i2)=dabs(rev(i,k))
        END DO
        CALL hinge(trev,i2,ts,xtmp,0)
        DO k=1,5
         revhng(i,k)=ts(k)
        END DO
       END DO
      END IF
      IF(Prttab(Nptr+1))THEN
       cobs='Hinge Values:'
       CALL prrvob(revhng,ncol,1,5,1,3,cobs,13,hdrttl,hdrptr,nhdrtl,
     &             hd2ttl,hd2ptr,nhd2tl,i0,tfmt1(1:npos),1,1,PNCHDR,9,
     &             tblttl(1:ntbttl),T)
       IF(Lfatal)RETURN
      END IF
c-----------------------------------------------------------------------
      IF(Svltab(LSLASF))WRITE(Ng,1050)'Seasonal',aartot(0,1)
      IF(Svltab(LSLASP))WRITE(Ng,1050)'Projected Seasonal',aartot(1,1)
      IF(Lsumm.gt.0)THEN
c-----------------------------------------------------------------------
       WRITE(Nform,1051)'r06.lag00.aar.all',aartot(0,1)
c-----------------------------------------------------------------------
       i2=0
       IF(Ny.eq.4)i2=12
       DO i=1,Ny
        IF(.not.dpeq(aarpd(0,i),DNOTST))THEN
         WRITE(brklbl,1049)'p',i
         WRITE(Nform,2049)'r06.lag00.',brklbl,cpobs(i+i2),aarpd(0,i)
        END IF
       END DO
c-----------------------------------------------------------------------
       i2=Revspn(YR)-1
       DO i=1,iyr
        IF(.not.dpeq(aaryr(0,i),DNOTST))THEN
         WRITE(brklbl,1049)'y',i
         WRITE(Nform,3049)'r06.lag00.',brklbl,i+i2,aaryr(0,i)
        END IF
       END DO
c-----------------------------------------------------------------------
       WRITE(Nform,1051)'r06.lag00.hinge.min',revhng(0,1)
       WRITE(Nform,1051)'r06.lag00.hinge.25p',revhng(0,2)
       WRITE(Nform,1051)'r06.lag00.hinge.med',revhng(0,3)
       WRITE(Nform,1051)'r06.lag00.hinge.75p',revhng(0,4)
       WRITE(Nform,1051)'r06.lag00.hinge.max',revhng(0,5)
c-----------------------------------------------------------------------
       WRITE(Nform,1051)'r06.proj.aar.all',aartot(1,1)
c-----------------------------------------------------------------------
       i2=0
       IF(Ny.eq.4)i2=12
       DO i=1,Ny
        IF(.not.dpeq(aarpd(1,i),DNOTST))THEN
         WRITE(brklbl,1049)'p',i
         WRITE(Nform,2049)'r06.proj.',brklbl,cpobs(i+i2),aarpd(1,i)
        END IF
       END DO
c-----------------------------------------------------------------------
       i2=Revspn(YR)-1
       DO i=1,iyr
        IF(.not.dpeq(aaryr(1,i),DNOTST))THEN
         WRITE(brklbl,1049)'y',i
         WRITE(Nform,3049)'r06.proj.',brklbl,i+i2,aaryr(1,i)
        END IF
       END DO
c-----------------------------------------------------------------------
       WRITE(Nform,1051)'r06.proj.hinge.min',revhng(1,1)
       WRITE(Nform,1051)'r06.proj.hinge.25p',revhng(1,2)
       WRITE(Nform,1051)'r06.proj.hinge.med',revhng(1,3)
       WRITE(Nform,1051)'r06.proj.hinge.75p',revhng(1,4)
       WRITE(Nform,1051)'r06.proj.hinge.max',revhng(1,5)
      END IF
c-----------------------------------------------------------------------
 1049 FORMAT('aar.',a,i2.2)
 2049 FORMAT(a,a,': ',a,' ',E17.10)
 3049 FORMAT(a,a,': ',i4,' ',E17.10)
 1050 FORMAT(' AveAbsRev of ',a,' : ',t40,f10.3)
 1051 FORMAT(a,': ',E17.10)
c-----------------------------------------------------------------------
c     If concurrent and final adjustments are to be printed, print out
c     title.
c-----------------------------------------------------------------------
      IF(Prttab(Nptr+2))THEN
c-----------------------------------------------------------------------
c     Print out header information
c-----------------------------------------------------------------------
       CALL makttl(DSDDIC,dsdptr,PDSD,Nptr+2,PDSUM6,tblttl,ntbttl,T,F)
       IF(Lfatal)RETURN
c-----------------------------------------------------------------------
       npos=34
       tbw=Tblwid
       IF(Tblwid.lt.10)tbw=10
       WRITE(tfmt1,1021)tbw,tbw
 1021  FORMAT('(1x,a5,1x,2(2(1x,f',i2,'.1),1x,f',i2,'.2))')
       WRITE(tfmt2,1031)tbw,tbw
 1031  FORMAT('(2x,i4,1x,2(2(1x,f',i2,'.1),1x,f',i2,'.2))')
c-----------------------------------------------------------------------
       i0=0
c-----------------------------------------------------------------------
c     Create data dictionary for column headers
c-----------------------------------------------------------------------
       CALL intlst(PNCHDR,hdrptr,nhdrtl)
       nhdr=nhdrtl+1
       CALL insstr('Revision',nhdr,PNCHDR,hdrttl,hdrptr,nhdrtl)
       IF(.not.Lfatal)
     &    CALL insstr('Final',nhdr,PNCHDR,hdrttl,hdrptr,nhdrtl)
       IF(.not.Lfatal)
     &    CALL insstr('Proj',nhdr,PNCHDR,hdrttl,hdrptr,nhdrtl)
       IF(.not.Lfatal)
     &    CALL insstr('Revision',nhdr,PNCHDR,hdrttl,hdrptr,nhdrtl)
       IF(.not.Lfatal)
     &    CALL insstr('Final',nhdr,PNCHDR,hdrttl,hdrptr,nhdrtl)
       IF(.not.Lfatal)
     &    CALL insstr('Conc',nhdr,PNCHDR,hdrttl,hdrptr,nhdrtl)
       IF(Lfatal)RETURN
c-----------------------------------------------------------------------
       CALL prtshd(tblttl(1:ntbttl),Revspn,Ny,Revnum,T)
       IF(Lfatal)RETURN
c-----------------------------------------------------------------------
       DO j=1,Revnum
        tmp(0,j)=Cnc(j)
        tmp(1,j)=Fin(j)
        tmp(2,j)=rev(0,j)
        tmp(3,j)=Cnc2(j)
        tmp(4,j)=Fin(j)
        tmp(5,j)=rev(1,j)
       END DO
c-----------------------------------------------------------------------
       end2=Endsa-1
       j=Revspn(YR)-1
       k=Begrev
       k2=Begrev+Ny-Revspn(MO)
       k3=Revspn(MO)
       DO WHILE (k.le.end2)
        IF(k2.gt.end2)k2=end2
        j=j+1
        ipos=1
        CALL itoc(j,cobs,ipos) 
        CALL prrvob(tmp,5,k-Begrev+1,k2-Begrev+1,1,1,cobs,ipos-1,
     &              hdrttl,hdrptr,nhdrtl,hd2ttl,hd2ptr,0,i0,
     &              tfmt1(1:npos),k3,5,PNCHDR,tbw,tblttl(1:ntbttl),
     &              F)
        IF(Lfatal)RETURN
        IF(k.eq.Begrev)k3=1
        k=k2+1
        k2=k2+Ny
       END DO
      END IF
c-----------------------------------------------------------------------
c     If percent revisions are to be saved, open file for saved
c     revisions.
c-----------------------------------------------------------------------
      IF(Savtab(Nptr))THEN
       CALL opnfil(T,F,Nptr,fh,locok)
       IF(.not.locok)THEN
        CALL abend
        RETURN
       END IF
c-----------------------------------------------------------------------
c     Print header for revisions
c-----------------------------------------------------------------------
       CALL intlst(PNCHDR,hdrptr,nhdrtl)
       nhdr=nhdrtl+1
       CALL insstr('PROJ_SF_revision',nhdr,PNCHDR,hdrttl,hdrptr,nhdrtl)
c-----------------------------------------------------------------------
       CALL intlst(PNCHDR,hdrptr,nhdrtl)
       nhdr=nhdrtl+1
       CALL insstr('SF_revision',nhdr,PNCHDR,hdrttl,hdrptr,nhdrtl)
c-----------------------------------------------------------------------
       IF(Lfatal)RETURN
       WRITE(fh,1040)'date',(TABCHR,
     &               hdrttl(hdrptr(ielt):hdrptr(ielt+1)-1),ielt=0,ncol)
       WRITE(fh,1040)'----',(TABCHR,'-----------------------',
     &               ielt=0,ncol)
c-----------------------------------------------------------------------
c     begin looping though observations
c-----------------------------------------------------------------------
       DO i=Begrev,Endsa-1
        Revptr=i-Begrev+1
c-----------------------------------------------------------------------
c     Set date of revision for observation Revptr
c-----------------------------------------------------------------------
        CALL addate(Revspn,Ny,Revptr-1,idate)
        rdbdat=100*idate(YR)+idate(MO)
c-----------------------------------------------------------------------
c     Save revision measure with date
c-----------------------------------------------------------------------
        ipos=1
        CALL itoc(rdbdat,outstr,ipos) 
        IF(Lfatal)RETURN
        DO k=0,ncol
         outstr(ipos:ipos)=TABCHR
         ipos=ipos+1
         CALL dtoc(rev(k,Revptr),outstr,ipos)
         IF(Lfatal)RETURN
        END DO
        WRITE(fh,1040)outstr(1:ipos-1)
       END DO
       CALL fclose(fh)
      END IF
c-----------------------------------------------------------------------
c     If concurrent and final adjustments are to be saved, open file for
c     saved adjustments.
c-----------------------------------------------------------------------
      IF(Savtab(Nptr+2).or.Lgraf)THEN
       IF(Savtab(Nptr+2))CALL opnfil(T,F,Nptr+2,fh,locok)
       IF(Lgraf)CALL opnfil(T,Lgraf,Nptr+2,fh2,locok)
       IF(.not.locok)THEN
        CALL abend
        RETURN
       END IF
c-----------------------------------------------------------------------
c     Print header for revisions
c-----------------------------------------------------------------------
       CALL intlst(3,hdrptr,nhdrtl)
       nhdr=nhdrtl+1
       ipos=1
       CALL insstr('Final_SF',nhdr,PNCHDR,hdrttl,hdrptr,nhdrtl)
       IF(.not.Lfatal)CALL insstr('Proj_SF',nhdr,3,hdrttl,hdrptr,
     &                            nhdrtl)
       IF(.not.Lfatal)CALL insstr('Conc_SF',nhdr,3,hdrttl,hdrptr,nhdrtl)
       IF(Lfatal)RETURN
       IF(Savtab(Nptr+2))THEN
        WRITE(fh,1040)'date',(TABCHR,
     &              hdrttl(hdrptr(ielt-1):hdrptr(ielt)-1),ielt=1,nhdrtl)
        WRITE(fh,1040)'----',(TABCHR,'-----------------------',
     &                ielt=1,nhdrtl)
       END IF
       IF(Lgraf)THEN
        WRITE(fh2,1040)'date',(TABCHR,
     &              hdrttl(hdrptr(ielt-1):hdrptr(ielt)-1),ielt=1,nhdrtl)
        WRITE(fh2,1040)'----',(TABCHR,'-----------------------',
     &                 ielt=1,nhdrtl)
       END IF
c-----------------------------------------------------------------------
c     begin looping though observations
c-----------------------------------------------------------------------
       DO i=Begrev,Endsa-1
        Revptr=i-Begrev+1
c-----------------------------------------------------------------------
c     Set date for ith revision
c-----------------------------------------------------------------------
        CALL addate(Revspn,Ny,Revptr-1,idate)
        rdbdat=100*idate(YR)+idate(MO)
c-----------------------------------------------------------------------
c     Save concurrent and final adjustments with date
c-----------------------------------------------------------------------
        ipos=1
        CALL itoc(rdbdat,outstr,ipos)
        IF(Lfatal)RETURN
        outstr(ipos:ipos)=TABCHR
        ipos=ipos+1
        CALL dtoc(Cnc(Revptr),outstr,ipos)
        IF(Lfatal)RETURN
        outstr(ipos:ipos)=TABCHR
        ipos=ipos+1
        CALL dtoc(Cnc2(Revptr),outstr,ipos)
        IF(Lfatal)RETURN
        outstr(ipos:ipos)=TABCHR
        ipos=ipos+1
        CALL dtoc(Fin(Revptr),outstr,ipos)
        IF(Lfatal)RETURN
        IF(Savtab(Nptr+2))WRITE(fh,1040)outstr(1:ipos-1)
        IF(Lgraf)WRITE(fh2,1040)outstr(1:ipos-1)
       END DO
       IF(Savtab(Nptr+2))CALL fclose(fh)
       IF(Lgraf)CALL fclose(fh2)
      END IF
c-----------------------------------------------------------------------
      RETURN
c-----------------------------------------------------------------------
 1040 FORMAT(1000a)
      END
