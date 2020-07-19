import { Injectable } from '@angular/core';
import { SafeUrl } from '@angular/platform-browser';

@Injectable({
  providedIn: 'root'
})
export class ContentService {
  contentText: string;
  filename: string;
  base64: SafeUrl;
  bytepos: number;
  nextindex: number;
  totallen: number;
  ipport: string;
  mediasource: string;

  constructor() { 
    this.contentText = "";
    this.filename = '';
    this.base64 ='';
    this.bytepos = 0;
    this.nextindex = 0;
    this.totallen = 0;
    this.ipport = "";
  }
}
