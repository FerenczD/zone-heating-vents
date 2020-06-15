import React, {Component} from 'react';

export default class HttpRequest extends Component {
    constructor(props) {
        super(props);
    }

    get = () => {
        fetch(this.props.url, {
            method: 'GET'
        })
            .then((response) => response.json())
            .then((responseJson) => {
                alert(JSON.stringify(responseJson));
                console.log(responseJson);
            })
            .catch((error) => {
                alert(JSON.stringify(error));
                console.error(error);
            });
    }
    post = () => {
        let dataToSend = {title: 'test', body: 'some data', messageId: 1};
        let formBody = [];
        for (let key in dataToSend) {
            let encodedKey = encodeURIComponent(key);
            let encodedValue = encodeURIComponent(dataToSend[key]);
            formBody.push(encodedKey + "=" + encodedValue);
        }
        formBody = formBody.join("&");
        fetch(this.props.url, {
            method: "POST",
            body: formBody,
            headers: {
                'Content-Type': 'application/x-www-form-urlencoded;charset=UTF-8'
            },
        })
            .then((response) => response.json())
            .then((responseJson) => {
                alert(JSON.stringify(responseJson));
                console.log(responseJson);
            })
            .catch((error) => {
                alert(JSON.stringify(error));
                console.error(error);
            });
    };
}
