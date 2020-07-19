import { Component, OnInit } from '@angular/core';
import { ContentService } from 'src/app/services/content.service';
import { LoginComponent } from '../login/login.component';
import {MatDialog, MatDialogRef} from '@angular/material';
import { Router } from '@angular/router';

@Component({
  selector: 'app-ip',
  templateUrl: './ip.component.html',
  styleUrls: ['./ip.component.scss']
})
export class IpComponent implements OnInit {

  constructor(public dialogRef: MatDialogRef<LoginComponent>,
    private router: Router,private datas: ContentService) { }

  ngOnInit() {
  }

  onSubmit(){
    console.log(this.datas.ipport);
    this.dialogRef.close();
  }

}
