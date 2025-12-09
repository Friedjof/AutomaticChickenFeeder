import './style.css';
import { ChickenFeederApp } from './app.js';

const startApp = () => {
    window.app = new ChickenFeederApp();
};

if (document.readyState === 'loading') {
    document.addEventListener('DOMContentLoaded', startApp);
} else {
    startApp();
}

window.addEventListener('beforeunload', () => {
    if (window.app) {
        window.app.destroy();
    }
});
