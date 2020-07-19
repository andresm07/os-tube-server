import { Component, OnInit } from '@angular/core';
import {MatDialog, MatDialogRef} from '@angular/material';
import { Router } from '@angular/router';
import { MediaService } from '../../services/media.service';
import {Multimedia} from '../../shared/models/multimedia.model'
import { DomSanitizer, SafeResourceUrl, SafeUrl} from '@angular/platform-browser';
import { ContentService } from 'src/app/services/content.service';
import { HttpResponse } from '@angular/common/http';
import { Res } from 'src/app/shared/models/res.model';

@Component({
  selector: 'app-main',
  templateUrl: './main.component.html',
  styleUrls: ['./main.component.scss']
})
export class MainComponent implements OnInit {

  list: Multimedia[];
  imageurl: any;
  videourl: string;
  mimeType = 'video/mp4';
  codecs= "H.264";
  ms = new MediaSource();
  sb: SourceBuffer;
  bytepos: number;
  total: number;
  nexti: number;
  video: any;
  firstbuffer: ArrayBuffer;
  videodir: string;

  constructor(private router: Router,
    private mediaService: MediaService,
     private sanitizer: DomSanitizer,
     private datas: ContentService) { }

  ngOnInit() {
    this.list = new Array;
    this.imageurl = "";
    this.mimeType = 'video/mp4';
    this.codecs = "H.264";
    this.ms = new MediaSource();
    this.bytepos = 0;
    this.nexti = 0;
    this.video = document.getElementById("video-example");
  }

  setMedia(list: Multimedia[]){
    this.list = list;
  }

  getData(){
    this.list = [];
    this.mediaService.getAllFiles().subscribe(
      (res: HttpResponse<Res>) => {
        res.body.data.map(
          (mult: Multimedia) => {
            new Multimedia().deserialize(mult);
            this.list.push(mult)
          }
        )
      }
    );
  }

  getResource(item: Multimedia){
    if(item.filetype.includes("image")){
      this.mediaService.requestFile(item.dir, 0).subscribe(
        (res: HttpResponse<ArrayBuffer>) => {
          const data = res.body;
          this.bytesToBase64(data, item.filetype);
          this.datas.filename = item.name;
          this.datas.contentText = "";
          this.datas.mediasource =  "";
          this.router.navigate(["/contentviewer"]);
        }
      );
    }else if(item.filetype.includes("plain")){
      this.mediaService.requestFile(item.dir, 0).subscribe(
        (res: HttpResponse<ArrayBuffer>) => {
          const data = res.body;
          this.datas.base64 = "";
          this.datas.filename = item.name;
          this.datas.mediasource =  "";
          this.datas.contentText = String.fromCharCode.apply(null, new Uint8Array(data));
          this.router.navigate(["/contentviewer"]);
        }
      );
    }else if(item.filetype.includes("video") || item.filetype.includes("audio")){
      this.videodir = item.dir;
      this.mediaService.requestFile(item.dir, 0).subscribe(
        (res: HttpResponse<ArrayBuffer>) => {
          const data = res.body;
          const headers = res.headers;
          this.firstbuffer = data;
          this.bytepos = parseInt(headers.get('Byte-Pos'));
          this.nexti = parseInt(headers.get('Next-Index'));
          this.total = parseInt(headers.get('Total-Bytes'));
          this.ms = new MediaSource();
          this.datas.filename = item.name;
          const url =  this.setUrl(data, item.filetype);
          console.log(url);
          this.datas.mediasource = url;
          this.datas.base64 = "";
          this.datas.contentText = "";
          (<HTMLVideoElement> this.video).src = url;
          (<HTMLVideoElement> this.video).addEventListener("progress", (event)=>{
            console.log(event.currentTarget);
          });
          //this.router.navigate(["/contentviewer"]);
          /*
          (<HTMLVideoElement> this.video).src = URL.createObjectURL(this.ms);
          this.ms.addEventListener('sourceopen', ()=>{
            this.mimeType = item.filetype;
            this.codecs= "avc1.4d400c";
            let value = item.filetype+ ';codecs="'+ this.codecs +'"';
            this.sb = this.ms.addSourceBuffer(value);
            console.log(this.ms.sourceBuffers.length);
            this.sb.appendBuffer(this.firstbuffer);
            this.sb.addEventListener("updateend", () =>{
              if(this.bytepos != this.total){
                this.getSegment();
              }
            }, false);
          }, false);
          this.ms.addEventListener('sourceclose', this.closed, false);
          //this.initvideo(data, item);*/
        }
      );
    }else if(item.filetype.includes("audio")){

    }else{
      this.mediaService.requestFile(item.dir, 0).subscribe(
        (res: HttpResponse<ArrayBuffer>) => {
          this.openfile(res.body, item.filetype)
        }
      );
    }
  }

  bytesToBase64(data: ArrayBuffer,contentType){
    let TYPED_ARRAY = new Uint8Array(data);
    const STRING_CHAR = TYPED_ARRAY.reduce(
      (datum, byte) =>{
        return datum + String.fromCharCode(byte);
      }, ''
    );
    let base64 = btoa(STRING_CHAR);
    this.datas.base64 = this.sanitizer.bypassSecurityTrustUrl('data:'+ contentType +';base64,' + base64);
    this.imageurl = "";
  }

  openfile(data: ArrayBuffer, contentType: string){
    let file = new Blob([data], { type: contentType });            
    var fileURL = URL.createObjectURL(file);
    window.open(fileURL);
  }

  setUrl(data: ArrayBuffer, contentType: string){
    let file = new Blob([data], { type: contentType });            
    return URL.createObjectURL(file);
  }

  initvideo(data: ArrayBuffer, item: Multimedia){
    const vid = document.getElementById("video-example");
    const mediaSource = new MediaSource();
    (<HTMLVideoElement>vid).src = URL.createObjectURL(mediaSource);
    this.videourl = URL.createObjectURL(mediaSource);
    mediaSource.addEventListener('sourceopen', () =>{
      console.log(mediaSource.readyState);
      const sourceBuffer = mediaSource.addSourceBuffer('video/mp4; codecs="avc1.42E01E, mp4a.40.2"');
      sourceBuffer.appendBuffer(data);
      sourceBuffer.addEventListener("updateend", () =>{
        mediaSource.endOfStream();
        (<HTMLVideoElement>vid).play();
      });
    });
  }

  opened(){
    this.mimeType = 'video/mp4';
    this.codecs= "avc1.4D401F";
    let value = this.mimeType+ ';codecs="'+ this.codecs +'"';
    this.sb = this.ms.addSourceBuffer(value);
    this.sb.appendBuffer(this.firstbuffer);
    this.sb.addEventListener("updateend", this.loadSegment);
  }

  loadSegment(){
    if(this.bytepos != this.total){
      this.getSegment();
    }
  }

  getSegment(){
    this.mediaService.requestFile(this.videodir, this.nexti).subscribe(
      (res: HttpResponse<ArrayBuffer>) => {
        const data = res.body;
        const headers = res.headers;
        this.firstbuffer = data;
        this.bytepos = parseInt(headers.get('Byte-Pos'));
        this.nexti = parseInt(headers.get('Next-Index'));
        this.total = parseInt(headers.get('Total-Bytes'));
        this.sb.appendBuffer(data);
      }
    );
  }

  closed(){

  }
}
