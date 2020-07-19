import { Routes } from '@angular/router';

import { HomeComponent } from '../../components/home/home.component';
import { MainComponent } from '../../components/main/main.component';
import { ContentviewerComponent } from 'src/app/components/contentviewer/contentviewer.component';

export const routes: Routes = [
    { path: 'home', component: HomeComponent},
    { path: 'main', component: MainComponent},
    { path: 'contentviewer', component: ContentviewerComponent},
    { path: '', redirectTo: '/home', pathMatch: 'full' }
];