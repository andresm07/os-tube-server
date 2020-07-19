import { Deserializable } from "./deserializable.model";

export class Multimedia implements Deserializable{
    name: string;
    filetype: string;
    dir: string;
    md5: string;

    deserialize(input: any) {
        Object.assign(this, input);
        return this;
    }
}