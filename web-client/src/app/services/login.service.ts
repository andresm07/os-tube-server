import { Injectable } from '@angular/core';
import { Observable, of } from 'rxjs';
import { delay } from 'rxjs/operators';
import { HttpClient, HttpHeaders } from '@angular/common/http';
import { baseURL } from '../shared/baseurl';
import { map, catchError } from 'rxjs/operators';
import { ProcessHTTPMsgService } from './process-httpmsg.service';
import { ContentService } from './content.service';

@Injectable({
  providedIn: 'root'
})
export class LoginService {

  constructor(private http: HttpClient, private processHTTPMsgService: ProcessHTTPMsgService,
    private datas: ContentService) { }

  loginClient(username: string, password: string): Observable<any> {
    const httpOptions = {
      headers: new HttpHeaders({
        'Content-Type': 'application/json'
      })
    };

    const loginBody = {
      "username": username,
      "password": password
    }

    return this.http.post<any>(`http://${this.datas.ipport}/login`, loginBody, {
      headers: new HttpHeaders({'Content-Type': 'application/json'}),
      observe: 'response'
    })
      .pipe(catchError(this.processHTTPMsgService.handleError));
  }

  logoutClient() {
    return this.http.get<any>(`http:${this.datas.ipport}/exit`)
      .pipe(catchError(this.processHTTPMsgService.handleError));
  }
}
