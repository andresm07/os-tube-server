import { Component, OnInit, Inject } from '@angular/core';
import {MatDialog, MatDialogRef} from '@angular/material';
import { Router } from '@angular/router';
import { RegisterService } from '../../services/register.service';
import { ContentService } from 'src/app/services/content.service';

@Component({
  selector: 'app-register',
  templateUrl: './register.component.html',
  styleUrls: ['./register.component.scss']
})
export class RegisterComponent implements OnInit {

  user = {
    username: "",
    password: "",
    remember: false
  }

  constructor(public dialogRef: MatDialogRef<RegisterComponent>, private router: Router,
    private registerService: RegisterService, @Inject('BaseURL') private BaseURL, private datas: ContentService) { }

  ngOnInit() {
  }

  onSubmit() {
    console.log("ipport", this.datas.ipport);
    //this.router.navigate(['/main']);
    this.registerService.registerClient(this.user.username, this.user.password)
      .subscribe((response: Response) => {
        const status = response.status;
        console.log(status);
        if(status == 200){
          this.router.navigate(['/main']);
        }
      });
    this.dialogRef.close();
  }

}
