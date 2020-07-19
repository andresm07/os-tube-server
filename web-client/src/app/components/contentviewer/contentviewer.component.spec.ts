import { async, ComponentFixture, TestBed } from '@angular/core/testing';

import { ContentviewerComponent } from './contentviewer.component';

describe('ContentviewerComponent', () => {
  let component: ContentviewerComponent;
  let fixture: ComponentFixture<ContentviewerComponent>;

  beforeEach(async(() => {
    TestBed.configureTestingModule({
      declarations: [ ContentviewerComponent ]
    })
    .compileComponents();
  }));

  beforeEach(() => {
    fixture = TestBed.createComponent(ContentviewerComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
