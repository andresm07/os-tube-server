import { Component, OnInit } from '@angular/core';
import { DomSanitizer, SafeResourceUrl, SafeUrl} from '@angular/platform-browser';
import { Router } from '@angular/router';
import { ContentService } from 'src/app/services/content.service';

@Component({
  selector: 'app-contentviewer',
  templateUrl: './contentviewer.component.html',
  styleUrls: ['./contentviewer.component.scss']
})
export class ContentviewerComponent implements OnInit {

  text: string;
  filename: string;
  imageurl: any;

  constructor(private router: Router, private sanitizer: DomSanitizer, 
    private datas: ContentService) { }

  ngOnInit() {
  }

  back(){
    this.router.navigate(['/main'])
  }

  

}
