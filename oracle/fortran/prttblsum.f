C     Last change:  BCM  19 Apr 2007   10:39 am
      SUBROUTINE prttblsum(Begdat,Sp,Y,Nobs,Srsttl,Outdec)
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
      CHARACTER blnk*80,cmonth*3,amonth*9,cqtr*3,fmt1*120,fmt2*120,
     &          thisOb*30,valuhd*5,cperiod*2
      INTEGER BTWNCL,blkwd,clwdth,i,ibeg,idate,idxwd,iend,irow,INCOL,
     &        istrt,itmp,j,mindec,MNSGFG,nblk,nblkln,nclprt,nclskp,ncol,
     &        ndec,nidxhd,nrows,nvalhd,Outdec,strtyr,ivec,mxtbl,nmonth
      PARAMETER(BTWNCL=3,INCOL=2,MNSGFG=3)
      DIMENSION cmonth(12),cqtr(4),idate(2),ivec(1),nmonth(12),
     &          amonth(12),cperiod(2)
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
      DATA amonth/'January  ','February ','March    ','April    ',
     &            '@        ','June     ','July     ','August   ',
     &            'September','October  ','November ','December '/
      DATA nmonth/7,8,5,5,1,4,4,6,9,7,8,8/
      DATA cqtr/'1st','2nd','3rd','4th'/
      DATA cperiod/' 1',' 2',' 3',' 4',' 5',' 6',' 7',' 8',' 9','10',
     &             '11','12','13'/
c-----------------------------------------------------------------------
c     Print the series title
c-----------------------------------------------------------------------
      Mxtbwd=80
*      IF(Lwdprt)Mxtbwd=132
      IF(len(Srsttl).gt.1)CALL writTagOneLine(Mt1,'h3','@',Srsttl)
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
*      clwdth=min(max(clwdth,3),15)
      WRITE(fmt1,1000)clwdth,ndec
 1000 FORMAT('(f',i2.2,'.',i1,')')
c-----------------------------------------------------------------------
c     copy y into yy vector (BCM April 2007)
c-----------------------------------------------------------------------
      thisNeg=.false.
      DO i=1,Nobs
       yy(i) = Y(i)
c-----------------------------------------------------------------------
c     For cases where ndec = 0 and the decimal fraction is exactly .5,
c     make an adjustment to ensure the number will round properly
c     when printed (BCM April 2007)
c-----------------------------------------------------------------------
       IF(dpeq(yy(i)-ceilng(yy(i)-0.5D0),0.5D0).and.ndec.eq.0)
     &     yy(i)=yy(i)+0.01D0
       IF(yy(i).lt.0.and.(.not.thisNeg))thisNeg=.true.
      END DO
      IF(thisNeg)clwdth=clwdth+1
      WRITE(fmt1,1000)clwdth,ndec
      WRITE(fmt2,1000)clwdth+2,ndec
 1000 FORMAT('(f',i2.2,'.',i1,')')
c-----------------------------------------------------------------------
      strtyr=Begdat(YR)
*      IF(Sp.eq.1)strtyr=strtyr-1
*      ivec(1)=strtyr+Nobs
*      CALL intfmt(ivec,1,idxwd)
*      idxwd=max(2,idxwd)
*      IF(idxwd.gt.3)THEN
*       nidxhd=4
*       idxhd(1:nidxhd)='Year'
*      ELSE
*       nidxhd=2
*       idxhd(1:nidxhd)='Yr'
*      END IF
      if(Sp.eq.1)THEN
       istrt=1
       CALL mkTableTag(Mt1,'w40',Srsttl)
       CALL mkCaption(Mt1,'w40',Srsttl)
      else
       istrt=Begdat(MO)
       CALL mkTableTag(Mt1,'x11',Srsttl)
       CALL mkCaption(Mt1,'x11',Srsttl)
      end if
      CALL writTag(Mt1,'<tr>')
      CALL mkTableCell(Mt1,'@','&nbsp;')
c-----------------------------------------------------------------------
      IF(Sp.eq.12)THEN
       do i = 1,Sp
        CALL mkHeaderCellScope(Mt1,0,0,'col',amonth(i)(1:nmonth(i)),
     &                         cmonth(i))
       end do
      ELSE IF (Sp.eq.4)THEN
       do i = 1,Sp
        CALL mkHeaderCellScope(Mt1,0,0,'col','@',cqtr(i)//' Quarter')
       end do
      ELSE
       do i = 1,Sp
        CALL mkHeaderCellScope(Mt1,0,0,'col','@','Period '//cperiod(i))
       end do
      END DO
      CALL mkHeaderCellScope(Mt1,0,0,'col','@','Total')
      CALL writTag(Mt1,'</tr>')
c-----------------------------------------------------------------------
c    print out first year
c-----------------------------------------------------------------------
      CALL writTag(Mt1,'<tr>')
      write(Mt1,1010)strtyr
 1010 FORMAT('<th scope="row">',i4,'</th>')
      if (istrt.gt.1)THEN
       DO i=1,istrt-1
        CALL mkTableCell(Mt1,'@','&nbsp;')
       END DO
      END IF
      iend=min(Sp-Begdat(2)+1,12,Nobs)
      DO i=istrt,tend
       write(thisOb,fmt1)yy(i)
       IF(yy(i).lt.0D0)THEN
        CALL mkTableCell(Mt1,'nowrap',thisOb)
       ELSE
        CALL mkTableCell(Mt1,'@',thisOb)
       END IF
      END DO
      CALL writTag(Mt1,'</tr>')
c-----------------------------------------------------------------------
c    Now print out the rest of the table
c-----------------------------------------------------------------------
      DO ibeg=iend+1,Nobs,Sp
       CALL writTag(Mt1,'<tr>')
       i2=ibeg+Sp-1
       iend=min(i2,Nobs)
       iyr=iyr+1
       write(Mt1,1010)iyr
       DO i=ibeg,iend
        write(thisOb,fmt1)yy(i)
        IF(Y(i).lt.0D0)THEN
         CALL mkTableCell(Mt1,'nowrap',thisOb)
        ELSE
         CALL mkTableCell(Mt1,'@',thisOb)
        END IF
       END DO
       IF (i2.gt.iend) THEN
        DO i=i2+1,iend
         CALL mkTableCell(Mt1,'@','&nbsp;')
        END DO
       END IF
       CALL writTag(Mt1,'</tr>')
      END DO
      CALL writTag(Mt1,'</table>')
      CALL mkPOneLine(Mt2,'@','&nbsp;')
c     ------------------------------------------------------------------
      RETURN
      END
