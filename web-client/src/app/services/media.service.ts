import { Injectable } from '@angular/core';
import { Observable, of } from 'rxjs';
import { delay } from 'rxjs/operators';
import { HttpClient, HttpHeaders } from '@angular/common/http';
import { baseURL } from '../shared/baseurl';
import { map, catchError } from 'rxjs/operators';
import { ProcessHTTPMsgService } from './process-httpmsg.service';
import {Multimedia} from '../shared/models/multimedia.model'
import { ContentService } from './content.service';

@Injectable({
  providedIn: 'root'
})
export class MediaService {

  constructor(private http: HttpClient, private processHTTPMsgService: ProcessHTTPMsgService,
    private datas: ContentService) { }

  requestFile(filename: string, bytePos: any) {
    return this.http.get(`http://${this.datas.ipport}/${filename}`, {
        headers: new HttpHeaders({
          'Byte-Pos': bytePos,
          'Client': 'web'
        }),
        responseType: 'arraybuffer',
        observe: 'response'
      })
      .pipe(catchError(this.processHTTPMsgService.handleError));
  }

  getAllFiles(): Observable<any> {
    return this.http.get<any>(`http://${this.datas.ipport}/res`, {observe: 'response'})
    .pipe(catchError(this.processHTTPMsgService.handleError));
    /*.pipe(
      map((res: Response) => 
        res.json()
      )
    );*/
  }
}
