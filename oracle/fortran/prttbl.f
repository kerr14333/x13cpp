C     Last change:  BCM  19 Apr 2007   10:39 am
      SUBROUTINE prttbl(Begdat,Sp,Y,Nobs,Srsttl,Outdec)
      IMPLICIT NONE
c-----------------------------------------------------------------------
c    Subroutine, prttbl, prints a table of monthly data.  What month
c the series begins in is adjusted for.
c-----------------------------------------------------------------------
c Parameters and include files
c Name  Type Description
c-----------------------------------------------------------------------
c one     d  Double precision 1
c pt5     d  Double precision .5
c zero    d  Double precision 0
c-----------------------------------------------------------------------
      INCLUDE 'srslen.prm'
      INCLUDE 'model.prm'
      INCLUDE 'units.cmn'
      INCLUDE 'title.cmn'
c     ------------------------------------------------------------------
      INTEGER Mxtbwd
c-----------------------------------------------------------------------
c Input Type Description
c-----------------------------------------------------------------------
c begdat  i  2 long array containing the year and period of the begining
c             date
c nobs    i  Number of observations to be printed
c sp      i  Seasonal period or sampling period
c srsttl  c  81 long character string for the input title of the series
c y       i  Nobs long vector of observations
c-----------------------------------------------------------------------
      CHARACTER Srsttl*(*)
      INTEGER Begdat,Nobs,Sp
      DOUBLE PRECISION Y,yy
      DIMENSION Begdat(2),Y(Nobs),yy(POBS)
c-----------------------------------------------------------------------
c Local Type Description
c-----------------------------------------------------------------------
c cmonth  c  Array of month abbreviations
c fmt1    c  String containing the format for the first year of data
c i       i  Index value
c ibeg    i  Index of begining observation on the current line
c iend    i  Index of last observation on the current line
c iyr     i  lndex for the year to be printed
c ncol    i  Number of columns in the printout
c-----------------------------------------------------------------------
      CHARACTER blnk*80,cmonth*3,cqtr*3,fmt1*120,fmt2*120,idxhd*4,
     &          valuhd*5
      INTEGER BTWNCL,blkwd,clwdth,i,ibeg,idate,idxwd,iend,irow,INCOL,
     &        istrt,itmp,j,mindec,MNSGFG,nblk,nblkln,nclprt,nclskp,ncol,
     &        ndec,nidxhd,nrows,nvalhd,Outdec,strtyr,ivec,mxtbl
      PARAMETER(BTWNCL=3,INCOL=2,MNSGFG=3)
      DIMENSION cmonth(12),cqtr(4),idate(2),ivec(1)
c-----------------------------------------------------------------------
      LOGICAL dpeq
      DOUBLE PRECISION ceilng
      EXTERNAL dpeq,ceilng
c-----------------------------------------------------------------------
      DATA blnk/
     &'                                                                 
     &               '/
      DATA cmonth/'Jan','Feb','Mar','Apr','May','Jun','Jul','Aug','Sep',
     &     'Oct','Nov','Dec'/
      DATA cqtr/'1st','2nd','3rd','4th'/
c-----------------------------------------------------------------------
c     Print the series title
c-----------------------------------------------------------------------
      Mxtbwd=80
      IF(Lwdprt)Mxtbwd=132
      IF(len(Srsttl).gt.1)THEN
       WRITE(Mt1,1010)Srsttl
 1010  FORMAT(/,' ',a)
      END IF
c-----------------------------------------------------------------------
c     Figure the column width and decimals
c-----------------------------------------------------------------------
      CALL numfmt(Y,Nobs,Outdec,clwdth,mindec)
      IF(mindec.gt.Outdec)THEN
       ndec=min(mindec+MNSGFG-1,11)
       clwdth=clwdth-Outdec+ndec
      ELSE
       ndec=Outdec
      END IF
      IF(ndec.eq.0)clwdth=clwdth+1
      clwdth=min(max(clwdth,3),15)
c-----------------------------------------------------------------------
c     copy y into yy vector (BCM April 2007)
c-----------------------------------------------------------------------
      DO i=1,Nobs
       yy(i) = Y(i)
c-----------------------------------------------------------------------
c     For cases where ndec = 0 and the decimal fraction is exactly .5,
c     make an adjustment to ensure the number will round properly
c     when printed (BCM April 2007)
c-----------------------------------------------------------------------
       IF(dpeq(yy(i)-ceilng(yy(i)-0.5D0),0.5D0).and.ndec.eq.0)
     &     yy(i)=yy(i)+0.01D0
      END DO
c-----------------------------------------------------------------------
      strtyr=Begdat(YR)
      IF(Sp.eq.1)strtyr=strtyr-1
      ivec(1)=strtyr+Nobs
      CALL intfmt(ivec,1,idxwd)
      idxwd=max(2,idxwd)
      IF(idxwd.gt.3)THEN
       nidxhd=4
       idxhd(1:nidxhd)='Year'
      ELSE
       nidxhd=2
       idxhd(1:nidxhd)='Yr'
      END IF
c-----------------------------------------------------------------------
c    Print out the heading, what there is of the first year of data,
c and calculate how many values were printed in the first year.
c-----------------------------------------------------------------------
      IF(Sp.eq.1)THEN
       blkwd=(BTWNCL+idxwd+INCOL+clwdth)
       ncol=(Mxtbwd-2)/blkwd
       ncol=min(ncol,int(sqrt(float(Nobs))))
       ncol=max(min(ncol,5),1)
       valuhd='Value'
       nvalhd=min(5,clwdth)
c     ------------------------------------------------------------------
       WRITE(Mt1,1020)'  ',('-',i=1,blkwd-BTWNCL),
     &                ((' ',i=1,BTWNCL),('-',i=1,blkwd-BTWNCL),j=2,ncol)
 1020  FORMAT(200(a))
       WRITE(Mt1,1030)blnk(1:2+idxwd-nidxhd),idxhd(1:nidxhd),
     &                blnk(1:INCOL+clwdth-nvalhd),valuhd(1:nvalhd),
     &                (blnk(1:BTWNCL+idxwd-nidxhd),idxhd(1:nidxhd),
     &                blnk(1:INCOL+clwdth-nvalhd),valuhd(1:nvalhd),i=2,
     &                ncol)
 1030  FORMAT(a,a,a,a,4(a,a,a,a))
       WRITE(Mt1,1020)'  ',('-',i=1,blkwd-BTWNCL),
     &                ((' ',i=1,BTWNCL),('-',i=1,blkwd-BTWNCL),j=2,ncol)
c     ------------------------------------------------------------------
       WRITE(fmt1,1040)2+idxwd,INCOL+clwdth,ndec,ncol-1,BTWNCL+idxwd,
     &                INCOL+clwdth,ndec
 1040  FORMAT('((i',i2.2,',f',i2.2,'.',i2.2,',:',i2,'(i',i2.2,',f',i2.2,
     &        '.',i2.2,')))')
c     ------------------------------------------------------------------
       nrows=(Nobs+ncol-1)/ncol
       DO irow=1,nrows
        WRITE(Mt1,fmt1)(strtyr+i,yy(i),i=irow,Nobs,nrows)
       END DO
c-----------------------------------------------------------------------
c     Tables for periodic data
c-----------------------------------------------------------------------
      ELSE
       IF(Sp.ge.6)THEN
        nblk=Sp
        istrt=Begdat(MO)
        iend=min(nblk-istrt+1,Nobs)
c     ------------------------------------------------------------------
        nvalhd=3
        itmp=INCOL+clwdth-nvalhd
c-----------------------------------------------------------------------
c     Decide on the number of columns
c-----------------------------------------------------------------------
        mxtbl=Mxtbwd-nidxhd-INCOL-2
        IF(Sp.ne.12)THEN
         ncol=min(5,mxtbl/(INCOL+clwdth))
        ELSE IF((Sp.ge.12.and.12*(INCOL+clwdth).le.mxtbl).or.
     &          (Sp*(INCOL+clwdth).le.mxtbl))THEN
         ncol=Sp
        ELSE IF(6*(INCOL+clwdth).le.mxtbl)THEN
         ncol=6
        ELSE
         ncol=4
        END IF
c     ------------------------------------------------------------------
        WRITE(Mt1,1050)'  ',
     &                 ('-',i=1,idxwd+BTWNCL+(ncol-1)*INCOL+ncol*clwdth)
 1050   FORMAT(200(a))
c     ------------------------------------------------------------------
        IF(Sp.eq.12)THEN
c     ------------------------------------------------------------------
         IF(ncol.lt.nblk)THEN
          WRITE(fmt2,1060)ncol-1
 1060     FORMAT('(a,a,a,',i2.2,'(a,a))')
          WRITE(Mt1,fmt2)blnk(1:2+idxwd),blnk(1:BTWNCL+clwdth-nvalhd),
     &                   cmonth(1),(blnk(1:itmp),cmonth(i),i=2,ncol)
          DO ibeg=ncol+1,nblk-ncol,ncol
           WRITE(Mt1,fmt2)blnk(1:2+idxwd),blnk(1:BTWNCL+clwdth-nvalhd),
     &                    cmonth(ibeg),
     &                    (blnk(1:itmp),cmonth(i),i=ibeg+1,ibeg+ncol-1)
          END DO
         END IF
c     ------------------------------------------------------------------
         ibeg=ncol*(nblk/ncol)+1
         IF(ibeg.gt.nblk)ibeg=nblk-ncol+1
         WRITE(fmt2,1070)ncol-1
 1070    FORMAT('(a,a,a,a,',i2.2,'(a,a))')
         WRITE(Mt1,fmt2)blnk(1:2+idxwd-nidxhd),idxhd(1:nidxhd),
     &                  blnk(1:BTWNCL+clwdth-nvalhd),cmonth(ibeg),
     &                  (blnk(1:itmp),cmonth(i),i=ibeg+1,nblk)
c     ------------------------------------------------------------------
        ELSE
         WRITE(Mt1,1080)blnk(1:2+idxwd),blnk(1:BTWNCL+clwdth-nvalhd),1,
     &                  (blnk(1:itmp),i,i=2,ncol)
 1080    FORMAT(a,a,i3,4(a,i3))
c     ------------------------------------------------------------------
         DO ibeg=ncol+1,nblk-ncol,ncol
          WRITE(Mt1,1080)blnk(1:2+idxwd),blnk(1:BTWNCL+clwdth-nvalhd),
     &                   ibeg,(blnk(1:itmp),i,i=ibeg+1,ibeg+ncol-1)
         END DO
c     ------------------------------------------------------------------
         ibeg=ncol*(nblk/ncol)+1
         IF(ibeg.gt.nblk)ibeg=nblk-ncol+1
         WRITE(Mt1,1090)blnk(1:2+idxwd-nidxhd),idxhd(1:nidxhd),
     &                  blnk(1:BTWNCL+clwdth-nvalhd),ibeg,
     &                  (blnk(1:itmp),i,i=ibeg+1,nblk)
 1090    FORMAT(a,a,a,i3,4(a,i3))
        END IF
c     ------------------------------------------------------------------
       ELSE
        ncol=Sp
        nblk=5*Sp
        IF(Sp.eq.4)THEN
         mxtbl=Mxtbwd-nidxhd-INCOL-2
         ncol=min(4,mxtbl/(INCOL+clwdth))
         nblk=Sp
        END IF
        istrt=Begdat(MO)
        iend=min(nblk-ncol*mod(Begdat(YR),5)-istrt+1,Nobs)
        IF(Sp.eq.4)iend=min(Sp-istrt+1,Nobs)
c     ------------------------------------------------------------------
        nvalhd=3
        itmp=INCOL+clwdth-nvalhd
c     ------------------------------------------------------------------
        WRITE(Mt1,1050)'  ',
     &                 ('-',i=1,idxwd+BTWNCL+(ncol-1)*INCOL+ncol*clwdth)
c     ------------------------------------------------------------------
        IF(Sp.eq.4)THEN
         WRITE(Mt1,1100)blnk(1:2+idxwd-nidxhd),idxhd(1:nidxhd),
     &                  blnk(1:itmp+BTWNCL-INCOL),cqtr(1),
     &                  (blnk(1:itmp),cqtr(i),i=2,4)
 1100    FORMAT(a,a,a,a,3(a,a))
c     ------------------------------------------------------------------
        ELSE
         WRITE(Mt1,1110)blnk(1:2+idxwd-nidxhd),idxhd(1:nidxhd),
     &                  blnk(1:itmp+BTWNCL-INCOL),1,
     &                  (blnk(1:itmp),i,i=2,Sp)
 1110    FORMAT(a,a,a,i3,5(a,i3))
        END IF
       END IF
c     ------------------------------------------------------------------
       WRITE(Mt1,1050)'  ',('-',i=1,idxwd+BTWNCL+(ncol-1)*INCOL+ncol*
     &                clwdth)
c     ------------------------------------------------------------------
       IF(istrt.gt.ncol)THEN
        nblkln=(istrt-1)/ncol
        nclskp=istrt-ncol*nblkln-1
        nclprt=min(nblk,ncol)-nclskp
        WRITE(fmt1,1120)2+idxwd,nblkln,2+idxwd+BTWNCL-INCOL+
     &                 nclskp*(INCOL+clwdth),nclprt,INCOL+clwdth,ndec,
     &                 2+idxwd+BTWNCL-INCOL,ncol,INCOL+clwdth,ndec
 1120   FORMAT('(i',i2.2,',',i1,'(/),',i2,'x,',i2,'f',i2.2,'.',i2.2,
     &         ':,/,(',i2,'x,:',i2,'f',i2.2,'.',i2.2,'))')
c     ------------------------------------------------------------------
       ELSE
        WRITE(fmt1,1130)2+idxwd,BTWNCL-INCOL+(istrt-1)*(INCOL+clwdth),
     &                 ncol-istrt+1,INCOL+clwdth,ndec,
     &                 2+idxwd+BTWNCL-INCOL,ncol,INCOL+clwdth,ndec
 1130   FORMAT('(i',i2.2,',',i3,'x,:',i2,'f',i2.2,'.',i2.2,',/,(',i2,
     &         'x,:',i2,'f',i2.2,'.',i2.2,'))')
       END IF
c     ------------------------------------------------------------------
       CALL addate(Begdat,Sp,0,idate)
       WRITE(Mt1,fmt1)strtyr,(yy(i),i=1,iend)
c-----------------------------------------------------------------------
c     Now print out the rest of the table
c-----------------------------------------------------------------------
       
       WRITE(fmt1,1140)2+idxwd,BTWNCL+clwdth,ndec,ncol-1,INCOL+clwdth,
     &                ndec,2+idxwd+BTWNCL-INCOL,ncol,INCOL+clwdth,ndec
 1140  FORMAT('(i',i2.2,',f',i2.2,'.',i2.2,',:',i2,'f',i2.2,'.',i2.2,
     &        '        ,/,(',i2,'x,:',i2,'f',i2.2,'.',i2.2,'))')
       CALL addate(idate,Sp,iend,idate)
c     ------------------------------------------------------------------
       DO ibeg=iend+1,Nobs,nblk
        iend=min(ibeg+nblk-1,Nobs)
*        IF(.not.Lcmpaq)WRITE(Mt1,'()')
        IF(.not.(Lcmpaq.or.Sp.eq.4))WRITE(Mt1,'()')
        WRITE(Mt1,fmt1)idate(YR),(yy(i),i=ibeg,iend)
        CALL addate(idate,Sp,iend-ibeg+1,idate)
       END DO
c     ------------------------------------------------------------------
       WRITE(Mt1,1050)'  ',('-',i=1,idxwd+BTWNCL+(ncol-1)*INCOL+ncol*
     &                clwdth)
      END IF
c     ------------------------------------------------------------------
      RETURN
      END
