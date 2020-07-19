import { async, ComponentFixture, TestBed } from '@angular/core/testing';

import { StreamviewerComponent } from './streamviewer.component';

describe('StreamviewerComponent', () => {
  let component: StreamviewerComponent;
  let fixture: ComponentFixture<StreamviewerComponent>;

  beforeEach(async(() => {
    TestBed.configureTestingModule({
      declarations: [ StreamviewerComponent ]
    })
    .compileComponents();
  }));

  beforeEach(() => {
    fixture = TestBed.createComponent(StreamviewerComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
