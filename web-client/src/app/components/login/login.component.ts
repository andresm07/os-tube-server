import { Component, OnInit, Inject } from '@angular/core';
import {MatDialog, MatDialogRef} from '@angular/material';
import { Router } from '@angular/router';
import { LoginService } from '../../services/login.service';
import { MediaService} from '../../services/media.service';
import {Multimedia} from '../../shared/models/multimedia.model'
import { ContentService } from 'src/app/services/content.service';


@Component({
  selector: 'app-login',
  templateUrl: './login.component.html',
  styleUrls: ['./login.component.scss']
})
export class LoginComponent implements OnInit {

  user = {
    username: "",
    password: "",
    remember: false
  }

  constructor(public dialogRef: MatDialogRef<LoginComponent>, private router: Router,
    private loginService: LoginService,private mservice: MediaService ,@Inject('BaseURL') private BaseURL, private datas: ContentService) { }

  ngOnInit() {
  }

  onSubmit() {
    //this.router.navigate(['/main']);
    this.loginService.loginClient(this.user.username, this.user.password)
      .subscribe((response: Response) => {
        const status = response.status;
        console.log(status);
        if(status == 200){
          this.router.navigate(['/main']);
        }
        //const statusCode = response.status;
        //console.log(statusCode);
      });
    this.dialogRef.close();
  }

}
